#!/usr/bin/env python
# -*- coding: utf-8-unix -*-

import numpy as np
from acidano.data_processing.utils.event_level import from_event_to_frame


def instrument_reconstruction(matrix, mapping):
    # Reconstruct an orchestral dictionnary pianoroll from
    #   - matrix : (time * pitch)
    #   - mapping : a dictionnary mapping index space of the matrix
    #               to the pitch space of each instrument

    pr_instru = {}

    max_velocity = 127
    # Dimensions of each instrument pianoroll
    T = matrix.shape[0]
    N = 128

    for instrument_name, ranges in mapping.iteritems():
        if instrument_name == 'Piano':
            continue
        index_min = ranges['index_min']
        index_max = ranges['index_max']
        pitch_min = ranges['pitch_min']
        pitch_max = ranges['pitch_max']

        this_pr = np.zeros((T, N), dtype=np.int16)
        this_pr[:, pitch_min:pitch_max] = matrix[:, index_min:index_max]
        this_pr = this_pr * max_velocity
        pr_instru[instrument_name] = this_pr

    return pr_instru


def instrument_reconstruction_piano(matrix, mapping):
    # Reconstruct an orchestral dictionnary pianoroll from
    #   - matrix : (time * pitch)
    #   - mapping : a dictionnary mapping index space of the matrix
    #               to the pitch space of each instrument

    pr_instru = {}

    max_velocity = 127
    # Dimensions of each instrument pianoroll
    T = matrix.shape[0]
    N = 128

    ranges = mapping['Piano']
    index_min = ranges['index_min']
    index_max = ranges['index_max']
    pitch_min = ranges['pitch_min']
    pitch_max = ranges['pitch_max']

    this_pr = np.zeros((T, N), dtype=np.int16)
    this_pr[:, pitch_min:pitch_max] = matrix[:, index_min:index_max]
    this_pr = this_pr * max_velocity
    pr_instru['Piano'] = this_pr

    return pr_instru
