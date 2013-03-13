#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Output controller gains to .cc and .h files.
#
# To run: ./generate_cgains.py
#

import os
import sys
import jinja2
import yaw_rate_controller as yrc

def generate(ccinfile, hinfile):
    c_data = yrc.design_controller()
    render_dict = {'NUMGAINS' : len(c_data),
                   'AROWS' : 5,
                   'ACOLS' : 5,
                   'BROWS' : 5,
                   'BCOLS' : 3,
                   'CROWS' : 1,
                   'CCOLS' : 5}
    GAINS = []
    SPEED = []
    for i in range(render_dict['NUMGAINS']):
        SPEED.append(c_data[i]['theta_R_dot'])
        A = c_data[i]['A_ce']
        B = c_data[i]['B_ce']
        C = c_data[i]['F']
        GAINS.append({'A_c' : A, 'B_c' : B, 'C_c' : C})
    render_dict['GAINS'] = GAINS
    render_dict['SPEED'] = SPEED
    generate_cc(ccinfile, render_dict)
    generate_h(hinfile, render_dict)


def generate_cc(filename, render_dict):
    template = template_setup(filename)
    outfile = name_outfile(filename, '.cpp')
    with open(outfile, 'w') as f:
        f.write(template.render(render_dict))

def generate_h(filename, render_dict):
    template = template_setup(filename)
    outfile = name_outfile(filename, '.h')
    with open(outfile, 'w') as f:
        f.write(template.render(render_dict))

def template_setup(filename):
    dirname, basename = os.path.split(os.path.abspath(filename))
    env = jinja2.Environment(loader=jinja2.FileSystemLoader(dirname),
                             undefined=jinja2.StrictUndefined,
                             trim_blocks=True)
    return env.get_template(basename)

def name_outfile(infile, ext):
    dirname, basename = os.path.split(os.path.abspath(infile))
    base0, ext0 = os.path.splitext(basename)
    base1, ext1 = os.path.splitext(base0)
    if ext0 == '.in' and ext1 == ext:
        return os.path.join(dirname, base0)
    if ext0 == ext:
        return os.path.join(dirname, base0 + '_gen' + ext0)
    return os.path.join(dirname, basename + '_gen' + ext)


if __name__ == '__main__':
    generate('cgains.cpp.in', 'cgains.h.in');
    sys.exit(0)