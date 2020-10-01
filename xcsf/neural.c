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
 * @file neural.c
 * @author Richard Preen <rpreen@gmail.com>
 * @copyright The Authors.
 * @date 2012--2020.
 * @brief An implementation of a multi-layer perceptron neural network.
 */

#include "neural.h"
#include "neural_layer_connected.h"
#include "neural_layer_dropout.h"
#include "neural_layer_noise.h"
#include "neural_layer_recurrent.h"
#include "neural_layer_softmax.h"

/**
 * @brief Initialises an empty neural network.
 * @param [in] xcsf The XCSF data structure.
 * @param [in] net The neural network to initialise.
 */
void
neural_init(const struct XCSF *xcsf, struct Net *net)
{
    (void) xcsf;
    net->head = NULL;
    net->tail = NULL;
    net->n_layers = 0;
    net->n_inputs = 0;
    net->n_outputs = 0;
    net->output = NULL;
}

/**
 * @brief Inserts a layer into a neural network.
 * @param [in] xcsf The XCSF data structure.
 * @param [in] net The neural network receiving the layer.
 * @param [in] l The layer to insert.
 * @param [in] pos The position in the network to insert the layer.
 */
void
neural_insert(const struct XCSF *xcsf, struct Net *net, struct Layer *l,
              const int pos)
{
    (void) xcsf;
    if (net->head == NULL || net->tail == NULL) { // empty list
        net->head = malloc(sizeof(struct Llist));
        net->head->layer = l;
        net->head->prev = NULL;
        net->head->next = NULL;
        net->tail = net->head;
        net->n_inputs = l->n_inputs;
        net->n_outputs = l->n_outputs;
        net->output = l->output;
    } else { // insert
        struct Llist *iter = net->tail;
        for (int i = 0; i < pos && iter != NULL; ++i) {
            iter = iter->prev;
        }
        struct Llist *new = malloc(sizeof(struct Llist));
        new->layer = l;
        new->prev = iter;
        if (iter == NULL) { // new head
            new->next = net->head;
            net->head->prev = new;
            net->head = new;
            net->n_outputs = l->n_outputs;
            net->output = l->output;
        } else {
            new->next = iter->next;
            iter->next = new;
            if (iter->next == NULL) { // new tail
                net->tail = new;
                net->n_inputs = l->n_inputs;
            } else { // middle
                new->next->prev = new;
            }
        }
    }
    ++(net->n_layers);
}

/**
 * @brief Removes a layer from a neural network.
 * @param [in] xcsf The XCSF data structure.
 * @param [in] net The neural network removing the layer.
 * @param [in] pos The position of the layer in the network to be removed.
 */
void
neural_remove(const struct XCSF *xcsf, struct Net *net, const int pos)
{
    // find the layer
    struct Llist *iter = net->tail;
    for (int i = 0; i < pos && iter != NULL; ++i) {
        iter = iter->prev;
    }
    if (iter == NULL) {
        printf("neural_layer_remove(): error finding layer to remove\n");
        exit(EXIT_FAILURE);
    } else if (iter->next == NULL && iter->prev == NULL) {
        printf("neural_layer_remove(): attempted to remove the only layer\n");
        exit(EXIT_FAILURE);
    }
    // head
    if (iter->prev == NULL) {
        net->head = iter->next;
        if (iter->next != NULL) {
            iter->next->prev = NULL;
        }
        net->output = net->head->layer->output;
        net->n_outputs = net->head->layer->n_outputs;
    }
    // tail
    if (iter->next == NULL) {
        net->tail = iter->prev;
        if (iter->prev != NULL) {
            iter->prev->next = NULL;
        }
    }
    // middle
    if (iter->prev != NULL && iter->next != NULL) {
        iter->next->prev = iter->prev;
        iter->prev->next = iter->next;
    }
    --(net->n_layers);
    layer_free(xcsf, iter->layer);
    free(iter->layer);
    free(iter);
}

/**
 * @brief Inserts a layer at the head of a neural network.
 * @param [in] xcsf The XCSF data structure.
 * @param [in] net The neural network receiving the layer.
 * @param [in] l The layer to insert.
 */
void
neural_push(const struct XCSF *xcsf, struct Net *net, struct Layer *l)
{
    neural_insert(xcsf, net, l, net->n_layers);
}

/**
 * @brief Removes the layer at the head of a neural network.
 * @param [in] xcsf The XCSF data structure.
 * @param [in] net The neural network receiving the layer.
 */
void
neural_pop(const struct XCSF *xcsf, struct Net *net)
{
    neural_remove(xcsf, net, net->n_layers - 1);
}

/**
 * @brief Copies a neural network.
 * @param [in] xcsf The XCSF data structure.
 * @param [in] dest The destination neural network.
 * @param [in] src The source neural network.
 */
void
neural_copy(const struct XCSF *xcsf, struct Net *dest, const struct Net *src)
{
    neural_init(xcsf, dest);
    const struct Llist *iter = src->tail;
    while (iter != NULL) {
        struct Layer *l = layer_copy(xcsf, iter->layer);
        neural_push(xcsf, dest, l);
        iter = iter->prev;
    }
}

/**
 * @brief Frees a neural network.
 * @param [in] xcsf The XCSF data structure.
 * @param [in] net The neural network to free.
 */
void
neural_free(const struct XCSF *xcsf, struct Net *net)
{
    struct Llist *iter = net->tail;
    while (iter != NULL) {
        layer_free(xcsf, iter->layer);
        free(iter->layer);
        net->tail = iter->prev;
        free(iter);
        iter = net->tail;
        --(net->n_layers);
    }
}

/**
 * @brief Randomises the layers within a neural network.
 * @param [in] xcsf The XCSF data structure.
 * @param [in] net The neural network to randomise.
 */
void
neural_rand(const struct XCSF *xcsf, const struct Net *net)
{
    const struct Llist *iter = net->tail;
    while (iter != NULL) {
        layer_rand(xcsf, iter->layer);
        iter = iter->prev;
    }
}

/**
 * @brief Mutates a neural network.
 * @param [in] xcsf The XCSF data structure.
 * @param [in] net The neural network to mutate.
 * @return Whether any alterations were made.
 */
_Bool
neural_mutate(const struct XCSF *xcsf, const struct Net *net)
{
    _Bool mod = false;
    _Bool do_resize = false;
    const struct Layer *prev = NULL;
    const struct Llist *iter = net->tail;
    while (iter != NULL) {
        const int orig_outputs = iter->layer->n_outputs;
        // if the previous layer has grown or shrunk this layer must be resized
        if (do_resize) {
            layer_resize(xcsf, iter->layer, prev);
            do_resize = false;
        }
        // mutate this layer
        if (layer_mutate(xcsf, iter->layer)) {
            mod = true;
        }
        // check if this layer changed size
        if (iter->layer->n_outputs != orig_outputs) {
            do_resize = true;
        }
        // move to next layer
        prev = iter->layer;
        iter = iter->prev;
    }
    return mod;
}

/**
 * @brief Resizes neural network layers as necessary.
 * @param [in] xcsf The XCSF data structure.
 * @param [in] net The neural network to resize.
 */
void
neural_resize(const struct XCSF *xcsf, const struct Net *net)
{
    const struct Layer *prev = NULL;
    const struct Llist *iter = net->tail;
    while (iter != NULL) {
        if (prev != NULL && iter->layer->n_inputs != prev->n_outputs) {
            layer_resize(xcsf, iter->layer, prev);
        }
        prev = iter->layer;
        iter = iter->prev;
    }
}

/**
 * @brief Forward propagates a neural network.
 * @param [in] xcsf The XCSF data structure.
 * @param [in] net The neural network to propagate.
 * @param [in] input The input state.
 */
void
neural_propagate(const struct XCSF *xcsf, const struct Net *net,
                 const double *input)
{
    const struct Llist *iter = net->tail;
    while (iter != NULL) {
        layer_forward(xcsf, iter->layer, input);
        input = layer_output(xcsf, iter->layer);
        iter = iter->prev;
    }
}

/**
 * @brief Performs a gradient descent update on a neural network.
 * @param [in] xcsf The XCSF data structure.
 * @param [in] net The neural network to be updated.
 * @param [in] truth The desired network output.
 * @param [in] input The input state.
 */
void
neural_learn(const struct XCSF *xcsf, const struct Net *net,
             const double *truth, const double *input)
{
    // reset deltas
    const struct Llist *iter = net->tail;
    while (iter != NULL) {
        memset(iter->layer->delta, 0, sizeof(double) * iter->layer->n_outputs);
        iter = iter->prev;
    }
    // calculate output layer delta
    const struct Layer *p = net->head->layer;
    for (int i = 0; i < p->n_outputs; ++i) {
        p->delta[i] = truth[i] - p->output[i];
    }
    // backward phase
    iter = net->head;
    while (iter != NULL) {
        const struct Layer *l = iter->layer;
        if (iter->next == NULL) {
            layer_backward(xcsf, l, input, 0);
        } else {
            const struct Layer *prev = iter->next->layer;
            layer_backward(xcsf, l, prev->output, prev->delta);
        }
        iter = iter->next;
    }
    // update phase
    iter = net->tail;
    while (iter != NULL) {
        layer_update(xcsf, iter->layer);
        iter = iter->prev;
    }
}

/**
 * @brief Returns the output of a specified neuron in the output layer of a
 * neural network.
 * @param [in] xcsf The XCSF data structure.
 * @param [in] net The neural network to output.
 * @param [in] IDX Which neuron in the output layer to return.
 * @return The output of the specified neuron.
 */
double
neural_output(const struct XCSF *xcsf, const struct Net *net, const int IDX)
{
    if (IDX < 0 || IDX >= net->n_outputs) {
        printf("neural_output(): error (%d) >= (%d)\n", IDX, net->n_outputs);
        exit(EXIT_FAILURE);
    }
    return layer_output(xcsf, net->head->layer)[IDX];
}

/**
 * @brief Returns the outputs from the output layer of a neural network.
 * @param [in] xcsf The XCSF data structure.
 * @param [in] net The neural network to output.
 * @return The neural network outputs.
 */
double *
neural_outputs(const struct XCSF *xcsf, const struct Net *net)
{
    return layer_output(xcsf, net->head->layer);
}

/**
 * @brief Prints a neural network.
 * @param [in] xcsf The XCSF data structure.
 * @param [in] net The neural network to print.
 * @param [in] print_weights Whether to print the weights in each layer.
 */
void
neural_print(const struct XCSF *xcsf, const struct Net *net,
             const _Bool print_weights)
{
    const struct Llist *iter = net->tail;
    int i = 0;
    while (iter != NULL) {
        printf("layer (%d) ", i);
        layer_print(xcsf, iter->layer, print_weights);
        iter = iter->prev;
        ++i;
    }
}

/**
 * @brief Returns the total number of non-zero weights in a neural network.
 * @param [in] xcsf The XCSF data structure.
 * @param [in] net A neural network.
 * @return The calculated network size.
 */
double
neural_size(const struct XCSF *xcsf, const struct Net *net)
{
    (void) xcsf;
    int size = 0;
    const struct Llist *iter = net->tail;
    while (iter != NULL) {
        const struct Layer *l = iter->layer;
        switch (l->layer_type) {
            case CONNECTED:
            case RECURRENT:
            case LSTM:
            case CONVOLUTIONAL:
                size += l->n_active;
                break;
            default:
                break;
        }
        iter = iter->prev;
    }
    return size;
}

/**
 * @brief Writes a neural network to a file.
 * @param [in] xcsf The XCSF data structure.
 * @param [in] net The neural network to save.
 * @param [in] fp Pointer to the file to be written.
 * @return The number of elements written.
 */
size_t
neural_save(const struct XCSF *xcsf, const struct Net *net, FILE *fp)
{
    size_t s = 0;
    s += fwrite(&net->n_layers, sizeof(int), 1, fp);
    s += fwrite(&net->n_inputs, sizeof(int), 1, fp);
    s += fwrite(&net->n_outputs, sizeof(int), 1, fp);
    const struct Llist *iter = net->tail;
    while (iter != NULL) {
        s += fwrite(&iter->layer->layer_type, sizeof(int), 1, fp);
        s += layer_save(xcsf, iter->layer, fp);
        iter = iter->prev;
    }
    return s;
}

/**
 * @brief Reads a neural network from a file.
 * @param [in] xcsf The XCSF data structure.
 * @param [in] net The neural network to load.
 * @param [in] fp Pointer to the file to be read.
 * @return The number of elements read.
 */
size_t
neural_load(const struct XCSF *xcsf, struct Net *net, FILE *fp)
{
    size_t s = 0;
    int nlayers = 0;
    int ninputs = 0;
    int noutputs = 0;
    s += fread(&nlayers, sizeof(int), 1, fp);
    s += fread(&ninputs, sizeof(int), 1, fp);
    s += fread(&noutputs, sizeof(int), 1, fp);
    neural_init(xcsf, net);
    for (int i = 0; i < nlayers; ++i) {
        struct Layer *l = malloc(sizeof(struct Layer));
        layer_init(l);
        s += fread(&l->layer_type, sizeof(int), 1, fp);
        layer_set_vptr(l);
        s += layer_load(xcsf, l, fp);
        neural_push(xcsf, net, l);
    }
    return s;
}
