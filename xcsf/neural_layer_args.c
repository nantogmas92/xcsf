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
 * @file neural_layer_args.c
 * @author Richard Preen <rpreen@gmail.com>
 * @copyright The Authors.
 * @date 2020.
 * @brief Functions operating on neural network arguments/constants.
 */

#include "neural_activations.h"
#include "neural_layer_avgpool.h"
#include "neural_layer_connected.h"
#include "neural_layer_convolutional.h"
#include "neural_layer_dropout.h"
#include "neural_layer_lstm.h"
#include "neural_layer_maxpool.h"
#include "neural_layer_noise.h"
#include "neural_layer_recurrent.h"
#include "neural_layer_softmax.h"
#include "neural_layer_upsample.h"
#include "utils.h"

/**
 * @brief Sets layer parameters to default values.
 * @param [in] args The layer parameters to initialise.
 */
void
layer_args_init(struct ArgsLayer *args)
{
    args->type = CONNECTED;
    args->n_inputs = 0;
    args->n_init = 0;
    args->n_max = 0;
    args->max_neuron_grow = 0;
    args->function = LOGISTIC;
    args->recurrent_function = LOGISTIC;
    args->height = 0;
    args->width = 0;
    args->channels = 0;
    args->size = 0;
    args->stride = 0;
    args->pad = 0;
    args->eta = 0;
    args->eta_min = 0;
    args->momentum = 0;
    args->decay = 0;
    args->probability = 0;
    args->scale = 0;
    args->evolve_weights = false;
    args->evolve_neurons = false;
    args->evolve_functions = false;
    args->evolve_eta = false;
    args->evolve_connect = false;
    args->sgd_weights = false;
    args->next = NULL;
}

/**
 * @brief Creates and returns a copy of specified layer parameters.
 * @param [in] src The layer parameters to be copied.
 */
struct ArgsLayer *
layer_args_copy(const struct ArgsLayer *src)
{
    struct ArgsLayer *new = malloc(sizeof(struct ArgsLayer));
    new->type = src->type;
    new->n_inputs = src->n_inputs;
    new->n_init = src->n_init;
    new->n_max = src->n_max;
    new->max_neuron_grow = src->max_neuron_grow;
    new->function = src->function;
    new->recurrent_function = src->recurrent_function;
    new->height = src->height;
    new->width = src->width;
    new->channels = src->channels;
    new->size = src->size;
    new->stride = src->stride;
    new->pad = src->pad;
    new->eta = src->eta;
    new->eta_min = src->eta_min;
    new->momentum = src->momentum;
    new->decay = src->decay;
    new->probability = src->probability;
    new->scale = src->scale;
    new->evolve_weights = src->evolve_weights;
    new->evolve_neurons = src->evolve_neurons;
    new->evolve_functions = src->evolve_functions;
    new->evolve_eta = src->evolve_eta;
    new->evolve_connect = src->evolve_connect;
    new->sgd_weights = src->sgd_weights;
    new->next = NULL;
    return new;
}

/**
 * @brief Prints layer input parameters.
 * @param [in] args The layer parameters to print.
 */
static void
layer_args_print_inputs(const struct ArgsLayer *args)
{
    if (layer_receives_images(args->type)) {
        if (args->height > 0) {
            printf(", height=%d", args->height);
        }
        if (args->width > 0) {
            printf(", width=%d", args->width);
        }
        if (args->channels > 0) {
            printf(", channels=%d", args->channels);
        }
        if (args->size > 0) {
            printf(", size=%d", args->size);
        }
        if (args->stride > 0) {
            printf(", stride=%d", args->stride);
        }
        if (args->pad > 0) {
            printf(", pad=%d", args->pad);
        }
    } else {
        printf(", n_inputs=%d", args->n_inputs);
    }
}

/**
 * @brief Prints layer gradient descent parameters.
 * @param [in] args The layer parameters to print.
 */
static void
layer_args_print_sgd(const struct ArgsLayer *args)
{
    if (args->sgd_weights) {
        printf(", sgd_weights=true");
        printf(", eta=%f", args->eta);
        if (args->evolve_eta) {
            printf(", evolve_eta=true");
            printf(", eta_min=%f", args->eta_min);
        } else {
            printf(", evolve_eta=false");
        }
        printf(", momentum=%f", args->momentum);
        if (args->decay > 0) {
            printf(", decay=%f", args->decay);
        }
    }
}

/**
 * @brief Prints layer evolutionary operator parameters.
 * @param [in] args The layer parameters to print.
 */
static void
layer_args_print_evo(const struct ArgsLayer *args)
{
    if (args->evolve_weights) {
        printf(", evolve_weights=true");
    }
    if (args->evolve_functions) {
        printf(", evolve_functions=true");
    }
    if (args->evolve_connect) {
        printf(", evolve_connect=true");
    }
    if (args->evolve_neurons) {
        printf(", evolve_neurons=true");
        printf(", n_max=%d", args->n_max);
        printf(", max_neuron_grow=%d", args->max_neuron_grow);
    }
}

/**
 * @brief Prints layer activation function parameters.
 * @param [in] args The layer parameters to print.
 */
static void
layer_args_print_activation(const struct ArgsLayer *args)
{
    switch (args->type) {
        case AVGPOOL:
        case MAXPOOL:
        case UPSAMPLE:
        case DROPOUT:
        case NOISE:
        case SOFTMAX:
            return;
        default:
            break;
    }
    printf(", activation=%s", neural_activation_string(args->function));
    if (args->type == LSTM) {
        printf(", recurrent_activation=%s",
               neural_activation_string(args->recurrent_function));
    }
}

/**
 * @brief Prints layer
 * @param [in] args The layer parameters to print.
 */
static bool
layer_args_print_scale(const struct ArgsLayer *args)
{
    bool cont = false;
    if (args->type == NOISE || args->type == DROPOUT) {
        printf(", probability=%f", args->probability);
        cont = true;
    }
    if (args->type == NOISE || args->type == SOFTMAX) {
        printf(", scale=%f", args->scale);
        cont = true;
    }
    if (args->type == MAXPOOL) {
        cont = true;
    }
    return cont;
}

/**
 * @brief Prints layer parameters.
 * @param [in] args The layer parameters to print.
 * @param [in] prefix String to prefix layer type.
 */
void
layer_args_print(struct ArgsLayer *args, const char *prefix)
{
    struct Net net; // create a temporary network to parse inputs
    neural_init(&net);
    neural_create(&net, args);
    neural_free(&net);
    int cnt = 0;
    for (const struct ArgsLayer *a = args; a != NULL; a = a->next) {
        printf(", %s_LAYER_%d={", prefix, cnt);
        ++cnt;
        printf("type=%s", layer_type_as_string(a->type));
        layer_args_print_activation(a);
        layer_args_print_inputs(a);
        if (layer_args_print_scale(a)) {
            printf("}");
            continue;
        }
        if (a->n_init > 0) {
            printf(", n_init=%d", a->n_init);
        }
        layer_args_print_evo(a);
        layer_args_print_sgd(a);
        printf("}");
    }
}

/**
 * @brief Frees memory used by a list of layer parameters and points to NULL.
 * @param [in] largs Pointer to the list of layer parameters to free.
 */
void
layer_args_free(struct ArgsLayer **largs)
{
    while (*largs != NULL) {
        struct ArgsLayer *arg = *largs;
        *largs = (*largs)->next;
        free(arg);
    }
}

/**
 * @brief Checks input layer arguments are valid.
 * @param [in] arg Input layer parameters.
 */
static void
layer_args_validate_inputs(struct ArgsLayer *arg)
{
    if (arg->type == DROPOUT || arg->type == NOISE) {
        if (arg->n_inputs < 1) {
            arg->n_inputs = arg->channels * arg->height * arg->width;
        } else if (arg->channels < 1 || arg->height < 1 || arg->width < 1) {
            arg->channels = 1;
            arg->height = 1;
            arg->width = arg->n_inputs;
        }
    }
    if (layer_receives_images(arg->type)) {
        if (arg->channels < 1) {
            printf("Error: input channels < 1\n");
            exit(EXIT_FAILURE);
        }
        if (arg->height < 1) {
            printf("Error: input height < 1\n");
            exit(EXIT_FAILURE);
        }
        if (arg->width < 1) {
            printf("Error: input width < 1\n");
            exit(EXIT_FAILURE);
        }
    } else if (arg->n_inputs < 1) {
        printf("Error: number of inputs < 1\n");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Checks network layer arguments are valid.
 * @param [in] args List of layer parameters to check.
 */
void
layer_args_validate(struct ArgsLayer *args)
{
    struct ArgsLayer *arg = args;
    if (arg == NULL) {
        printf("Error: empty layer argument list\n");
        exit(EXIT_FAILURE);
    }
    layer_args_validate_inputs(arg);
    do {
        if (arg->evolve_neurons && arg->max_neuron_grow < 1) {
            printf("Error: evolving neurons but max_neuron_grow < 1\n");
            exit(EXIT_FAILURE);
        }
        if (arg->n_max < arg->n_init) {
            arg->n_max = arg->n_init;
        }
        arg = arg->next;
    } while (arg != NULL);
}

/**
 * @brief Returns the current output layer arguments.
 * @param [in] head Head of the list of layer parameters.
 * @return Layer parameters pertaining to the current output layer.
 */
struct ArgsLayer *
layer_args_tail(struct ArgsLayer *head)
{
    struct ArgsLayer *tail = head;
    while (tail->next != NULL) {
        tail = tail->next;
    }
    return tail;
}

/**
 * @brief Returns a bitstring representing the permissions granted by a layer.
 * @param [in] args Layer initialisation parameters.
 * @return Bitstring representing the layer options.
 */
uint32_t
layer_args_opt(const struct ArgsLayer *args)
{
    uint32_t lopt = 0;
    if (args->evolve_eta) {
        lopt |= LAYER_EVOLVE_ETA;
    }
    if (args->sgd_weights) {
        lopt |= LAYER_SGD_WEIGHTS;
    }
    if (args->evolve_weights) {
        lopt |= LAYER_EVOLVE_WEIGHTS;
    }
    if (args->evolve_neurons) {
        lopt |= LAYER_EVOLVE_NEURONS;
    }
    if (args->evolve_functions) {
        lopt |= LAYER_EVOLVE_FUNCTIONS;
    }
    if (args->evolve_connect) {
        lopt |= LAYER_EVOLVE_CONNECT;
    }
    return lopt;
}

/**
 * @brief Returns the length of the neural network layer parameter list.
 * @param [in] args Layer initialisation parameters.
 * @return The list length.
 */
static int
layer_args_length(const struct ArgsLayer *args)
{
    int n = 0;
    const struct ArgsLayer *iter = args;
    while (iter != NULL) {
        iter = iter->next;
        ++n;
    }
    return n;
}

/**
 * @brief Saves neural network layer parameters.
 * @param [in] args Layer initialisation parameters.
 * @param [in] fp Pointer to the output file.
 * @return The total number of elements written.
 */
size_t
layer_args_save(const struct ArgsLayer *args, FILE *fp)
{
    size_t s = 0;
    const int n = layer_args_length(args);
    s += fwrite(&n, sizeof(int), 1, fp);
    const struct ArgsLayer *iter = args;
    while (iter != NULL) {
        s += fwrite(&iter->type, sizeof(int), 1, fp);
        s += fwrite(&iter->n_inputs, sizeof(int), 1, fp);
        s += fwrite(&iter->n_init, sizeof(int), 1, fp);
        s += fwrite(&iter->n_max, sizeof(int), 1, fp);
        s += fwrite(&iter->max_neuron_grow, sizeof(int), 1, fp);
        s += fwrite(&iter->function, sizeof(int), 1, fp);
        s += fwrite(&iter->recurrent_function, sizeof(int), 1, fp);
        s += fwrite(&iter->height, sizeof(int), 1, fp);
        s += fwrite(&iter->width, sizeof(int), 1, fp);
        s += fwrite(&iter->channels, sizeof(int), 1, fp);
        s += fwrite(&iter->size, sizeof(int), 1, fp);
        s += fwrite(&iter->stride, sizeof(int), 1, fp);
        s += fwrite(&iter->pad, sizeof(int), 1, fp);
        s += fwrite(&iter->eta, sizeof(double), 1, fp);
        s += fwrite(&iter->eta_min, sizeof(double), 1, fp);
        s += fwrite(&iter->momentum, sizeof(double), 1, fp);
        s += fwrite(&iter->decay, sizeof(double), 1, fp);
        s += fwrite(&iter->probability, sizeof(double), 1, fp);
        s += fwrite(&iter->scale, sizeof(double), 1, fp);
        s += fwrite(&iter->evolve_weights, sizeof(bool), 1, fp);
        s += fwrite(&iter->evolve_neurons, sizeof(bool), 1, fp);
        s += fwrite(&iter->evolve_functions, sizeof(bool), 1, fp);
        s += fwrite(&iter->evolve_eta, sizeof(bool), 1, fp);
        s += fwrite(&iter->evolve_connect, sizeof(bool), 1, fp);
        s += fwrite(&iter->sgd_weights, sizeof(bool), 1, fp);
        iter = iter->next;
    }
    return s;
}

/**
 * @brief Loads neural network layer parameters.
 * @param [in] largs Pointer to the list of layer parameters to load.
 * @param [in] fp Pointer to the output file.
 * @return The total number of elements read.
 */
size_t
layer_args_load(struct ArgsLayer **largs, FILE *fp)
{
    layer_args_free(largs);
    size_t s = 0;
    int n = 0;
    s += fread(&n, sizeof(int), 1, fp);
    for (int i = 0; i < n; ++i) {
        struct ArgsLayer *arg = malloc(sizeof(struct ArgsLayer));
        layer_args_init(arg);
        s += fread(&arg->type, sizeof(int), 1, fp);
        s += fread(&arg->n_inputs, sizeof(int), 1, fp);
        s += fread(&arg->n_init, sizeof(int), 1, fp);
        s += fread(&arg->n_max, sizeof(int), 1, fp);
        s += fread(&arg->max_neuron_grow, sizeof(int), 1, fp);
        s += fread(&arg->function, sizeof(int), 1, fp);
        s += fread(&arg->recurrent_function, sizeof(int), 1, fp);
        s += fread(&arg->height, sizeof(int), 1, fp);
        s += fread(&arg->width, sizeof(int), 1, fp);
        s += fread(&arg->channels, sizeof(int), 1, fp);
        s += fread(&arg->size, sizeof(int), 1, fp);
        s += fread(&arg->stride, sizeof(int), 1, fp);
        s += fread(&arg->pad, sizeof(int), 1, fp);
        s += fread(&arg->eta, sizeof(double), 1, fp);
        s += fread(&arg->eta_min, sizeof(double), 1, fp);
        s += fread(&arg->momentum, sizeof(double), 1, fp);
        s += fread(&arg->decay, sizeof(double), 1, fp);
        s += fread(&arg->probability, sizeof(double), 1, fp);
        s += fread(&arg->scale, sizeof(double), 1, fp);
        s += fread(&arg->evolve_weights, sizeof(bool), 1, fp);
        s += fread(&arg->evolve_neurons, sizeof(bool), 1, fp);
        s += fread(&arg->evolve_functions, sizeof(bool), 1, fp);
        s += fread(&arg->evolve_eta, sizeof(bool), 1, fp);
        s += fread(&arg->evolve_connect, sizeof(bool), 1, fp);
        s += fread(&arg->sgd_weights, sizeof(bool), 1, fp);
        if (*largs == NULL) {
            *largs = arg;
        } else {
            struct ArgsLayer *iter = *largs;
            while (iter->next != NULL) {
                iter = iter->next;
            }
            iter->next = arg;
        }
    }
    return s;
}
