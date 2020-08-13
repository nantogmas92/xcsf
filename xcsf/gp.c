/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file gp.c
 * @author Richard Preen <rpreen@gmail.com>
 * @copyright The Authors.
 * @date 2016--2020.
 * @brief An implementation of GP trees based upon TinyGP.
 * @see Poli, Langdon, and McPhee (2008) "A Field Guide to Genetic Programming"
 */

#include "gp.h"
#include "sam.h"
#include "utils.h"

#define GP_MAX_LEN (10000) //!< Maximum length of a GP tree.
#define GP_NUM_FUNC (4) //!< Number of selectable GP functions
#define ADD (0)
#define SUB (1)
#define MUL (2)
#define DIV (3)

#define N_MU (1) //!< Number of tree-GP mutation rates
static const int MU_TYPE[N_MU] = {
    SAM_RATE_SELECT
}; //<! Self-adaptation method

/**
 * @brief Traverses a GP tree.
 * @param tree The tree to traverse.
 * @param p The position from which to traverse.
 * @return The position after traversal.
 */
static int
tree_traverse(int *tree, int p)
{
    if (tree[p] >= GP_NUM_FUNC) {
        ++p;
        return (p);
    }
    switch (tree[p]) {
        case ADD:
        case SUB:
        case MUL:
        case DIV:
            ++p;
            return (tree_traverse(tree, tree_traverse(tree, p)));
        default:
            printf("tree_traverse() invalid function: %d\n", tree[p]);
            exit(EXIT_FAILURE);
    }
}

/**
 * @brief Grows a random GP tree of specified max length and depth.
 * @details Only used to create an initial tree.
 * @param xcsf The XCSF data structure.
 * @param buffer Buffer to hold the flattened GP tree generated.
 * @param p The position from which to traverse (start at 0).
 * @param max Maximum tree length.
 * @param depth Maximum tree depth.
 * @return The position after traversal (i.e., tree length).
 */
static int
tree_grow(const struct XCSF *xcsf, int *buffer, int p, int max, int depth)
{
    int prim = irand_uniform(0, 2);
    if (p >= max) {
        return (-1);
    }
    if (p == 0) {
        prim = 1;
    }
    if (prim == 0 || depth == 0) {
        prim = irand_uniform(GP_NUM_FUNC,
                             GP_NUM_FUNC + xcsf->GP_NUM_CONS + xcsf->x_dim);
        buffer[p] = prim;
        return (p + 1);
    } else {
        prim = irand_uniform(0, GP_NUM_FUNC);
        switch (prim) {
            case ADD:
            case SUB:
            case MUL:
            case DIV:
                buffer[p] = prim;
                int one_child = tree_grow(xcsf, buffer, p + 1, max, depth - 1);
                if (one_child < 0) {
                    return (-1);
                }
                return (tree_grow(xcsf, buffer, one_child, max, depth - 1));
            default:
                printf("tree_grow() invalid function: %d\n", prim);
                exit(EXIT_FAILURE);
        }
    }
}

/**
 * @brief Initialises the constants shared among all GP trees.
 * @param xcsf The XCSF data structure.
 */
void
tree_init_cons(struct XCSF *xcsf)
{
    xcsf->gp_cons = malloc(sizeof(double) * xcsf->GP_NUM_CONS);
    for (int i = 0; i < xcsf->GP_NUM_CONS; ++i) {
        xcsf->gp_cons[i] = rand_uniform(xcsf->COND_MIN, xcsf->COND_MAX);
    }
}

/**
 * @brief Frees the constants shared among all GP trees.
 * @param xcsf The XCSF data structure.
 */
void
tree_free_cons(const struct XCSF *xcsf)
{
    free(xcsf->gp_cons);
}

/**
 * @brief Creates a random GP tree.
 * @param xcsf The XCSF data structure.
 * @param gp The GP tree being randomised.
 */
void
tree_rand(const struct XCSF *xcsf, struct GP_TREE *gp)
{
    int buffer[GP_MAX_LEN];
    gp->len = 0;
    do {
        gp->len = tree_grow(xcsf, buffer, 0, GP_MAX_LEN, xcsf->GP_INIT_DEPTH);
    } while (gp->len < 0);
    gp->tree = malloc(sizeof(int) * gp->len);
    memcpy(gp->tree, buffer, sizeof(int) * gp->len);
    gp->mu = malloc(sizeof(double) * N_MU);
    sam_init(gp->mu, N_MU, MU_TYPE);
}

/**
 * @brief Frees a GP tree.
 * @param xcsf The XCSF data structure.
 * @param gp The GP tree to free.
 */
void
tree_free(const struct XCSF *xcsf, const struct GP_TREE *gp)
{
    (void) xcsf;
    free(gp->tree);
    free(gp->mu);
}

/**
 * @brief Evaluates a GP tree.
 * @param xcsf The XCSF data structure.
 * @param gp The GP tree to evaluate.
 * @param x The input state.
 * @return The result from evaluating the GP tree.
 */
double
tree_eval(const struct XCSF *xcsf, struct GP_TREE *gp, const double *x)
{
    int node = gp->tree[gp->p];
    ++(gp->p);
    if (node >= GP_NUM_FUNC + xcsf->GP_NUM_CONS) {
        return (x[node - GP_NUM_FUNC - xcsf->GP_NUM_CONS]);
    } else if (node >= GP_NUM_FUNC) {
        return (xcsf->gp_cons[node - GP_NUM_FUNC]);
    }
    switch (node) {
        case ADD:
            return (tree_eval(xcsf, gp, x) + tree_eval(xcsf, gp, x));
        case SUB:
            return (tree_eval(xcsf, gp, x) - tree_eval(xcsf, gp, x));
        case MUL:
            return (tree_eval(xcsf, gp, x) * tree_eval(xcsf, gp, x));
        case DIV: {
            double num = tree_eval(xcsf, gp, x);
            double den = tree_eval(xcsf, gp, x);
            if (den == 0) {
                return (num);
            }
            return (num / den);
        }
        default:
            printf("eval() invalid function: %d\n", node);
            exit(EXIT_FAILURE);
    }
}

/**
 * @brief Prints a GP tree.
 * @param xcsf The XCSF data structure.
 * @param gp The GP tree to print.
 * @param p The position from which to traverse (start at 0).
 * @return The position after traversal.
 */
int
tree_print(const struct XCSF *xcsf, const struct GP_TREE *gp, int p)
{
    int node = gp->tree[p];
    if (node >= GP_NUM_FUNC) {
        if (node >= GP_NUM_FUNC + xcsf->GP_NUM_CONS) {
            printf("IN:%d ", node - GP_NUM_FUNC - xcsf->GP_NUM_CONS);
        } else {
            printf("%f", xcsf->gp_cons[node - GP_NUM_FUNC]);
        }
        ++p;
        return (p);
    }
    printf("(");
    ++p;
    int a1 = tree_print(xcsf, gp, p);
    switch (node) {
        case ADD:
            printf(" + ");
            break;
        case SUB:
            printf(" - ");
            break;
        case MUL:
            printf(" * ");
            break;
        case DIV:
            printf(" / ");
            break;
        default:
            printf("tree_print() invalid function: %d\n", node);
            exit(EXIT_FAILURE);
    }
    int a2 = tree_print(xcsf, gp, a1);
    printf(")");
    return a2;
}

/**
 * @brief Copies a GP tree.
 * @param xcsf The XCSF data structure.
 * @param dest The destination GP tree.
 * @param src The source GP tree.
 */
void
tree_copy(const struct XCSF *xcsf, struct GP_TREE *dest,
          const struct GP_TREE *src)
{
    (void) xcsf;
    dest->len = src->len;
    dest->tree = malloc(sizeof(int) * src->len);
    memcpy(dest->tree, src->tree, sizeof(int) * src->len);
    dest->p = src->p;
    dest->mu = malloc(sizeof(double) * N_MU);
    memcpy(dest->mu, src->mu, sizeof(double) * N_MU);
}

/**
 * @brief Performs sub-tree crossover.
 * @param xcsf The XCSF data structure.
 * @param p1 The first GP tree to perform crossover.
 * @param p2 The second GP tree to perform crossover.
 */
void
tree_crossover(const struct XCSF *xcsf, struct GP_TREE *p1, struct GP_TREE *p2)
{
    int len1 = p1->len;
    int len2 = p2->len;
    int start1 = irand_uniform(0, len1);
    int end1 = tree_traverse(p1->tree, start1);
    int start2 = irand_uniform(0, len2);
    int end2 = tree_traverse(p2->tree, start2);
    int nlen1 = start1 + (end2 - start2) + (len1 - end1);
    int *new1 = malloc(sizeof(int) * nlen1);
    memcpy(&new1[0], &p1->tree[0], sizeof(int) * start1);
    memcpy(&new1[start1], &p2->tree[start2], sizeof(int) * (end2 - start2));
    memcpy(&new1[start1 + (end2 - start2)], &p1->tree[end1],
           sizeof(int) * (len1 - end1));
    int nlen2 = start2 + (end1 - start1) + (len2 - end2);
    int *new2 = malloc(sizeof(int) * nlen2);
    memcpy(&new2[0], &p2->tree[0], sizeof(int) * start2);
    memcpy(&new2[start2], &p1->tree[start1], sizeof(int) * (end1 - start1));
    memcpy(&new2[start2 + (end1 - start1)], &p2->tree[end2],
           sizeof(int) * (len2 - end2));
    tree_free(xcsf, p1);
    tree_free(xcsf, p2);
    p1->tree = new1;
    p2->tree = new2;
    p1->len = tree_traverse(p1->tree, 0);
    p2->len = tree_traverse(p2->tree, 0);
}

/**
 * @brief Performs point mutation on a GP tree.
 * @details Terminals are randomly replaced with other terminals and functions
 * are randomly replaced with other functions.
 * @param xcsf The XCSF data structure.
 * @param gp The GP tree to be mutated.
 * @return Whether any alterations were made.
 */
_Bool
tree_mutate(const struct XCSF *xcsf, struct GP_TREE *gp)
{
    _Bool changed = false;
    sam_adapt(gp->mu, N_MU, MU_TYPE);
    int terminal_max = GP_NUM_FUNC + xcsf->GP_NUM_CONS + xcsf->x_dim;
    for (int i = 0; i < gp->len; ++i) {
        if (rand_uniform(0, 1) < gp->mu[0]) {
            changed = true;
            if (gp->tree[i] >= GP_NUM_FUNC) {
                gp->tree[i] = irand_uniform(GP_NUM_FUNC, terminal_max);
            } else {
                gp->tree[i] = irand_uniform(0, GP_NUM_FUNC);
            }
        }
    }
    return changed;
}

/**
 * @brief Writes the GP tree to a binary file.
 * @param xcsf The XCSF data structure.
 * @param gp The GP tree to save.
 * @param fp Pointer to the file to be written.
 * @return The number of elements written.
 */
size_t
tree_save(const struct XCSF *xcsf, const struct GP_TREE *gp, FILE *fp)
{
    (void) xcsf;
    size_t s = 0;
    s += fwrite(&gp->p, sizeof(int), 1, fp);
    s += fwrite(&gp->len, sizeof(int), 1, fp);
    s += fwrite(gp->tree, sizeof(int), gp->len, fp);
    s += fwrite(gp->mu, sizeof(double), N_MU, fp);
    return s;
}

/**
 * @brief Reads a GP tree from a binary file.
 * @param xcsf The XCSF data structure.
 * @param gp The GP tree to load.
 * @param fp Pointer to the file to be read.
 * @return The number of elements read.
 */
size_t
tree_load(const struct XCSF *xcsf, struct GP_TREE *gp, FILE *fp)
{
    (void) xcsf;
    size_t s = 0;
    s += fread(&gp->p, sizeof(int), 1, fp);
    s += fread(&gp->len, sizeof(int), 1, fp);
    if (gp->len < 1) {
        printf("tree_load(): read error\n");
        gp->len = 1;
        exit(EXIT_FAILURE);
    }
    gp->tree = malloc(sizeof(int) * gp->len);
    s += fread(gp->tree, sizeof(int), gp->len, fp);
    s += fread(gp->mu, sizeof(double), N_MU, fp);
    return s;
}
