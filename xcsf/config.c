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
 * @file config.c
 * @author Richard Preen <rpreen@gmail.com>
 * @copyright The Authors.
 * @date 2015--2019.
 * @brief Configuration file handling functions.
 */ 

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <errno.h>
#include "xcsf.h"
#include "gp.h"
#include "config.h"
#include "loss.h"

#define MAXLEN 127 //!< Maximum config file line length to read
#define BASE 10 //!< Decimal numbers

/**
 * @brief Configuration file parameter data structure.
 */ 
typedef struct PARAM_LIST {
    char *name; //!< Parameter name
    char *value; //!< Parameter value
    struct PARAM_LIST *next; //!< Pointer to the next parameter
} PARAM_LIST;

static PARAM_LIST *head; //!< Linked list of config file parameters
static void init_config(const char *filename);
static void process(char *configline);
static void trim(char *s);
static void newnvpair(const char *config);
static char *getvalue(char *name);
static void tidyup();

/**
 * @brief Initialises global constants and reads the specified configuration file.
 * @param xcsf The XCSF data structure.
 * @param filename The name of the config file to read.
 */
void constants_init(XCSF *xcsf, const char *filename)
{
    init_config(filename);
    char *end;
    // integers
    xcsf->ACT_TYPE = strtoimax(getvalue("ACT_TYPE"), &end, BASE);
    xcsf->COND_TYPE = strtoimax(getvalue("COND_TYPE"), &end, BASE);
    xcsf->DGP_NUM_NODES = strtoimax(getvalue("DGP_NUM_NODES"), &end, BASE);
    xcsf->GP_INIT_DEPTH = strtoimax(getvalue("GP_INIT_DEPTH"), &end, BASE);
    xcsf->GP_NUM_CONS = strtoimax(getvalue("GP_NUM_CONS"), &end, BASE);
    xcsf->LOSS_FUNC = strtoimax(getvalue("LOSS_FUNC"), &end, BASE);
    xcsf->MAX_K = strtoimax(getvalue("MAX_K"), &end, BASE);
    xcsf->MAX_T = strtoimax(getvalue("MAX_T"), &end, BASE);
    xcsf->MAX_TRIALS = strtoimax(getvalue("MAX_TRIALS"), &end, BASE);
    xcsf->COND_NUM_HIDDEN_NEURONS = strtoimax(getvalue("COND_NUM_HIDDEN_NEURONS"), &end, BASE);
    xcsf->COND_MAX_HIDDEN_NEURONS = strtoimax(getvalue("COND_MAX_HIDDEN_NEURONS"), &end, BASE);
    xcsf->COND_HIDDEN_NEURON_ACTIVATION = strtoimax(getvalue("COND_HIDDEN_NEURON_ACTIVATION"), &end, BASE);
    xcsf->PRED_NUM_HIDDEN_NEURONS = strtoimax(getvalue("PRED_NUM_HIDDEN_NEURONS"), &end, BASE);
    xcsf->PRED_MAX_HIDDEN_NEURONS = strtoimax(getvalue("PRED_MAX_HIDDEN_NEURONS"), &end, BASE);
    xcsf->PRED_HIDDEN_NEURON_ACTIVATION = strtoimax(getvalue("PRED_HIDDEN_NEURON_ACTIVATION"), &end, BASE);
    xcsf->OMP_NUM_THREADS = strtoimax(getvalue("OMP_NUM_THREADS"), &end, BASE);
    xcsf->PERF_AVG_TRIALS = strtoimax(getvalue("PERF_AVG_TRIALS"), &end, BASE);
    xcsf->POP_SIZE = strtoimax(getvalue("POP_SIZE"), &end, BASE);
    xcsf->PRED_TYPE = strtoimax(getvalue("PRED_TYPE"), &end, BASE);
    xcsf->SAM_NUM = strtoimax(getvalue("SAM_NUM"), &end, BASE);
    xcsf->SAM_TYPE = strtoimax(getvalue("SAM_TYPE"), &end, BASE);
    xcsf->LAMBDA = strtoimax(getvalue("LAMBDA"), &end, BASE);
    xcsf->EA_SELECT_TYPE = strtoimax(getvalue("EA_SELECT_TYPE"), &end, BASE);
    xcsf->THETA_SUB = strtoimax(getvalue("THETA_SUB"), &end, BASE);
    xcsf->THETA_DEL = strtoimax(getvalue("THETA_DEL"), &end, BASE);
    xcsf->TELETRANSPORTATION = strtoimax(getvalue("TELETRANSPORTATION"), &end, BASE);
    // floats
    xcsf->ALPHA = atof(getvalue("ALPHA")); 
    xcsf->BETA = atof(getvalue("BETA"));
    xcsf->DELTA = atof(getvalue("DELTA"));
    xcsf->EPS_0 = atof(getvalue("EPS_0"));
    xcsf->ERR_REDUC = atof(getvalue("ERR_REDUC"));
    xcsf->FIT_REDUC = atof(getvalue("FIT_REDUC"));
    xcsf->INIT_ERROR = atof(getvalue("INIT_ERROR"));
    xcsf->INIT_FITNESS = atof(getvalue("INIT_FITNESS"));
    xcsf->NU = atof(getvalue("NU"));
    xcsf->THETA_EA = atof(getvalue("THETA_EA"));
    xcsf->EA_SELECT_SIZE = atof(getvalue("EA_SELECT_SIZE"));
    xcsf->P_CROSSOVER = atof(getvalue("P_CROSSOVER"));
    xcsf->F_MUTATION = atof(getvalue("F_MUTATION"));
    xcsf->P_MUTATION = atof(getvalue("P_MUTATION"));
    xcsf->S_MUTATION = atof(getvalue("S_MUTATION"));
    xcsf->E_MUTATION = atof(getvalue("E_MUTATION"));
    xcsf->SAM_MIN = atof(getvalue("SAM_MIN"));
    xcsf->COND_MAX = atof(getvalue("COND_MAX"));
    xcsf->COND_MIN = atof(getvalue("COND_MIN"));
    xcsf->COND_SMIN = atof(getvalue("COND_SMIN"));
    xcsf->COND_ETA = atof(getvalue("COND_ETA"));
    xcsf->PRED_RLS_LAMBDA = atof(getvalue("PRED_RLS_LAMBDA"));
    xcsf->PRED_RLS_SCALE_FACTOR = atof(getvalue("PRED_RLS_SCALE_FACTOR"));
    xcsf->PRED_X0 = atof(getvalue("PRED_X0"));
    xcsf->PRED_ETA = atof(getvalue("PRED_ETA"));
    xcsf->PRED_MOMENTUM = atof(getvalue("PRED_MOMENTUM"));
    xcsf->GAMMA = atof(getvalue("GAMMA"));
    xcsf->P_EXPLORE = atof(getvalue("P_EXPLORE"));
    // Bools
    xcsf->POP_INIT = false;
    if(strncmp(getvalue("POP_INIT"), "true", 4) == 0) {
        xcsf->POP_INIT = true;
    }
    xcsf->EA_SUBSUMPTION = false;
    if(strncmp(getvalue("EA_SUBSUMPTION"), "true", 4) == 0) {
        xcsf->EA_SUBSUMPTION = true;
    }
    xcsf->SET_SUBSUMPTION = false;
    if(strncmp(getvalue("SET_SUBSUMPTION"), "true", 4) == 0) {
        xcsf->SET_SUBSUMPTION = true;
    }
    xcsf->RESET_STATES = false;
    if(strncmp(getvalue("RESET_STATES"), "true", 4) == 0) {
        xcsf->RESET_STATES = true;
    }
    xcsf->COND_EVOLVE_WEIGHTS = false;
    if(strncmp(getvalue("COND_EVOLVE_WEIGHTS"), "true", 4) == 0) {
        xcsf->COND_EVOLVE_WEIGHTS = true;
    }
    xcsf->COND_EVOLVE_NEURONS = false;
    if(strncmp(getvalue("COND_EVOLVE_NEURONS"), "true", 4) == 0) {
        xcsf->COND_EVOLVE_NEURONS = true;
    }
    xcsf->COND_EVOLVE_FUNCTIONS = false;
    if(strncmp(getvalue("COND_EVOLVE_FUNCTIONS"), "true", 4) == 0) {
        xcsf->COND_EVOLVE_FUNCTIONS = true;
    }
    xcsf->PRED_EVOLVE_WEIGHTS = false;
    if(strncmp(getvalue("PRED_EVOLVE_WEIGHTS"), "true", 4) == 0) {
        xcsf->PRED_EVOLVE_WEIGHTS = true;
    }
    xcsf->PRED_EVOLVE_NEURONS = false;
    if(strncmp(getvalue("PRED_EVOLVE_NEURONS"), "true", 4) == 0) {
        xcsf->PRED_EVOLVE_NEURONS = true;
    }
    xcsf->PRED_EVOLVE_FUNCTIONS = false;
    if(strncmp(getvalue("PRED_EVOLVE_FUNCTIONS"), "true", 4) == 0) {
        xcsf->PRED_EVOLVE_FUNCTIONS = true;
    }
    xcsf->PRED_EVOLVE_ETA = false;
    if(strncmp(getvalue("PRED_EVOLVE_ETA"), "true", 4) == 0) {
        xcsf->PRED_EVOLVE_ETA = true;
    }
    xcsf->PRED_SGD_WEIGHTS = false;
    if(strncmp(getvalue("PRED_SGD_WEIGHTS"), "true", 4) == 0) {
        xcsf->PRED_SGD_WEIGHTS = true;
    }
    xcsf->PRED_RESET = false;
    if(strncmp(getvalue("PRED_RESET"), "true", 4) == 0) {
        xcsf->PRED_RESET = true;
    }
    // initialise (shared) tree-GP constants
    tree_init_cons(xcsf);
    // initialise loss/error function
    loss_set_func(xcsf);
    // clean up
    tidyup();
}

/**
 * @brief Frees all global constants.
 * @param xcsf The XCSF data structure.
 */
void constants_free(XCSF *xcsf) 
{
    tree_free_cons(xcsf);
}

/**
 * @brief Removes tabs/spaces/lf/cr from both ends of a line.
 * @param s The line to trim.
 */
static void trim(char *s)
{
    size_t i = 0;
    while(s[i]==' ' || s[i]=='\t' || s[i] =='\n' || s[i]=='\r') {
        i++;
    }
    if(i > 0) {
        size_t j = 0;
        while(j < strnlen(s, MAXLEN)) {
            s[j] = s[j+i];
            j++;
        }
        s[j] = '\0';
    }
    i = strnlen(s, MAXLEN)-1;
    while(s[i]==' ' || s[i]=='\t' || s[i] =='\n' || s[i]=='\r') {
        i--;
    }
    if(i < (strnlen(s, MAXLEN)-1)) {
        s[i+1] = '\0';
    }
}

/**
 * @brief Adds a parameter to the list.
 * @param config The parameter to add.
 */
static void newnvpair(const char *config) {
    // first pair
    if(head == NULL) {
        head = malloc(sizeof(PARAM_LIST));
        head->next = NULL;
    }
    // other pairs
    else {
        PARAM_LIST *new = malloc(sizeof(PARAM_LIST));
        new->next = head;
        head = new;
    }
    // get length of name
    size_t namelen = 0; // length of name
    _Bool err = true;
    for(namelen = 0; namelen < strnlen(config, MAXLEN); namelen++) {
        if(config[namelen] == '=') {
            err = false;
            break;
        }
    }
    // no = found
    if(err) {
        printf("error reading config: no '=' found\n");
        exit(EXIT_FAILURE);
    }
    // get name
    char *name = malloc(namelen+1);
    snprintf(name, namelen+1, "%s", config);
    // get value
    size_t valuelen = strnlen(config,MAXLEN)-namelen; // length of value
    char *value = malloc(valuelen);
    snprintf(value, valuelen, "%s", config+namelen+1);
    // add pair
    head->name = name;
    head->value = value;
}

/**
 * @brief Returns the value of a specified parameter from the list.
 * @param name The name of the parameter.
 * @return The value of the parameter.
 */
static char *getvalue(char *name) {
    char *result = NULL;
    for(PARAM_LIST *iter = head; iter != NULL; iter = iter->next) {
        if(strcmp(name, iter->name) == 0) {
            result = iter->value;
            break;
        }
    }
    return result;
}

/**
 * @brief Parses a line of the config file and adds to the list.
 * @param configline A single line of the configuration file.
 */
static void process(char *configline) {
    // ignore empty lines
    if(strnlen(configline, MAXLEN) == 0) {
        return;
    }
    // lines starting with # are comments
    if(configline[0] == '#') {
        return; 
    }
    // remove anything after #
    char *ptr = strchr(configline, '#');
    if(ptr != NULL) {
        *ptr = '\0';
    }
    newnvpair(configline);
}

/**
 * @brief Reads the specified configuration file.
 * @param filename The name of the configuration file.
 */
static void init_config(const char *filename) {
    FILE *f = fopen(filename, "rt");
    if(f == NULL) {
        printf("ERROR: cannot open %s\n", filename);
        return;
    }
    char buff[MAXLEN];
    head = NULL;
    while(!feof(f)) {
        if(fgets(buff, MAXLEN-2, f) == NULL) {
            break;
        }
        trim(buff);
        process(buff);
    }
    fclose(f);
}

/**
 * @brief Frees the config file parameter list.
 */
static void tidyup()
{ 
    PARAM_LIST *iter = head;
    while(iter != NULL) {
        free(head->value);
        free(head->name);
        head = iter->next;
        free(iter);
        iter = head;
    }    
    head = NULL;
}
