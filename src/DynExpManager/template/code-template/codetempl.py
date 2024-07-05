#!/usr/bin/python3

'''
codetempl.py

Created on: 28 Apr 2017
    Author: Fabian Meyer, Julian Bopp
'''

import sys
import re
import argparse
import getpass
import datetime
import subprocess
import json
import os.path

VERSION = '0.3.1'
HOME_DIR = os.path.expanduser("~")
ESC_MAP = {
    '$': r'\$'
}
VAR_USER = {}
KEYWORDS = ['if', 'else', 'endif', 'foreach', 'endfor']


def get_process_output(args, cwd=None):
    proc = subprocess.Popen(args,
        stdout=subprocess.PIPE, cwd=cwd)

    return proc.stdout.read().decode('ascii')


def get_date(cfg, filepath, param):
    fmt = '%d %b %Y' if 'fmt' not in param else param['fmt']
    now = datetime.datetime.now()
    return now.strftime(fmt)


def get_user(cfg, filepath, param):
    return getpass.getuser()


def get_gituser(cfg, filepath, param):
    path = os.path.dirname(filepath)
    return get_process_output(['git', 'config', 'user.name'], path).strip()


def get_gitemail(cfg, filepath, param):
    path = os.path.dirname(filepath)
    return get_process_output(['git', 'config', 'user.email'], path).strip()


def get_filename(cfg, filepath, param):
    return os.path.basename(filepath)


def get_filepath(cfg, filepath, param):
    return filepath


def get_guard(cfg, filepath, param):
    lvl = 0 if 'lvl' not in param else param['lvl']
    name = os.path.basename(filepath)
    name, ext = os.path.splitext(name)
    ext = ext.replace('.', '')

    regexalphanum = re.compile('[^\w]+')

    basedir = os.path.dirname(filepath)
    result = ['', ext, name]
    for i in range(lvl):
        basedir, subdir = os.path.split(basedir)
        subdir = regexalphanum.sub('', subdir)
        result.append(subdir)

    # reverse list and concatenate elements
    result.reverse()
    result = '_'.join(result).upper()
    return result


VAR_FUNCS = {
    'date': get_date,
    'user': get_user,
    'gituser': get_gituser,
    'gitemail': get_gitemail,
    'filename': get_filename,
    'filepath': get_filepath,
    'guard': get_guard
}


def map_by_colon(arglist):
    result = {}

    for arg in arglist:
        splits = arg.split(":")
        if len(splits) != 2:
            print('Error: invalid mapping {}'.format(arg))
            sys.exit(1)

        result[splits[0]] = splits[1]

    return result


def opts_from_file(filename):
    result = []
    with open(filename) as f:
        linenr = 0
        for line in f.readlines():
            linenr += 1

            line = line.strip()
            # check if line is empty or is a comment
            if len(line) == 0 or line[0] == '#':
                continue

            opts = [line]
            # find separating space
            idx = line.find(' ')
            if idx > 0:
                opts = [line[:idx].strip(), line[idx:].strip()]

            if opts[0][0] != '-':
                if len(opts[0]) == 1:
                    opts[0] = '-' + opts[0]
                else:
                    opts[0] = '--' + opts[0]

            result.extend(opts)

    return result


def parse_arguments():
    global_cfg = os.path.join(HOME_DIR, '.codetemplrc')

    parser = argparse.ArgumentParser(prog='codetempl',
        description='Generate code files from templates.')
    parser.add_argument('files', nargs='+',
        help='files to create')
    parser.add_argument('--config', help='read configuration from file')
    parser.add_argument('-v', '--version', help='shows version number',
        action='version',
        version=('codetempl Version ' + VERSION))
    parser.add_argument('--esc-char',
        dest='esc',
        help='escape character for template variables',
        default='$')
    parser.add_argument('--map-ext',
        dest='map_ext',
        help='map file extension to template files',
        action='append',
        default=[])
    parser.add_argument('--search-dir',
        dest='search_dir',
        help='search directories for template files',
        action='append',
        default=[])
    parser.add_argument('--user-var',
        dest='user_var',
        help='user defined variables',
        action='append',
        default=[])
    parser.add_argument('--vars-json',
        dest='vars_json',
        help='user defined variables to be loaded from a JSON file',
        default='')
    parser.add_argument('-f', '--force',
        dest='force',
        help='force overwrite of files',
        action='store_true')
    parser.add_argument('-e', '--extract-vars-json',
        dest='extract_vars_json',
        help='list variables occurring in specified template files',
        action='store_true')

    cfg = parser.parse_args()

    # args that will be used for the final parsing
    myargs = sys.argv[1:]
    # check if there was a config file specified
    if cfg.config is not None:
        myargs = opts_from_file(cfg.config) + myargs
    # check if there is a global config file
    if os.path.exists(global_cfg):
        myargs = opts_from_file(global_cfg) + myargs

    # parse arguments
    cfg = parser.parse_args(myargs)

    # parse the file extension mapping
    cfg.map_ext = map_by_colon(cfg.map_ext)

    # parse user defined variables
    global VAR_USER
    tmp = map_by_colon(cfg.user_var)
    for k, v in tmp.items():
        VAR_USER[k.lower()] = v

    # load user defined variables from a JSON file
    if cfg.vars_json != '':
        VAR_USER = {**VAR_USER, **load_user_vars_from_json(cfg.vars_json)}

    return cfg


def load_user_vars_from_json(file):
    vars = {}

    with open(file) as json_file: 
        vars = json.load(json_file)
        
    # all keys to lowercase
    vars = dict((k.lower(), v) for k, v in vars.items())

    return vars


def get_esc(cfg):
    if cfg.esc in ESC_MAP:
        return ESC_MAP[cfg.esc]
    else:
        return cfg.esc

    
def replace_conditions(content, cfg, filepath):
    regexcond = re.compile(r'{0}{0}if\s*\((.*?)\)\s*^(.*?)(?:{0}{0}else\s*^(.*?))?{0}{0}endif\s?'.format(get_esc(cfg)), re.MULTILINE | re.DOTALL)
    result = content

    # start matching all conditions if(cond)...[else...]endif
    m = regexcond.search(result)
    while m is not None:
        parsed_cond = replace_vars(m.group(1), cfg, filepath, True)
        val = m.group(2) if eval(parsed_cond) else (m.group(3) if m.group(3) is not None else '')

        result = integrate_block(result, val, m.start(), m.end())
        m = regexcond.search(result, m.start())

    return result

    
def replace_loops(content, cfg, filepath):
    esc = get_esc(cfg)
    regexfor = re.compile(r'{0}{0}foreach\s*\(\s*{0}(\w+){0}?\s*\)\s*^(.*?){0}{0}endfor\s?'.format(esc), re.MULTILINE | re.DOTALL)
    result = content

    # start matching all loops foreach(list)...endfor
    m = regexfor.search(result)
    while m is not None:
        var = m.group(1)
        varlow = var.lower()
        val = '<foreach failed>'

        if varlow not in VAR_USER:
            print('Warning: unknown variable {}'.format(var))
        elif not isinstance(VAR_USER[varlow], list):
            print('Warning: variable {} is not a list'.format(var))
        else:
            val = ''

            for element in VAR_USER[varlow]:
                val = val + re.sub(r'{0}{1}{0}?'.format(esc, varlow), element, m.group(2), flags = re.IGNORECASE)
        
        result = integrate_block(result, val, m.start(), m.end())
        m = regexfor.search(result, m.start())

    return result


# in case of empty pattern: remove surplus newline(s) in text
def integrate_block(text, pattern, start, end):
    if pattern == '' and start >= 2 and text[start - 1] == '\n' and text[start - 2] == '\n' and text[end] == '\n':
        text = text[:start - 1] + text[end:]
    else:
        text = text[:start] + pattern + text[end:]

    return text;


def replace_vars(content, cfg, filepath, make_boolean_evaluable = False):
    # regex for alphanumeric identfiers starting with esc character
    regexvar = re.compile(r'{0}(\w+){0}?'.format(get_esc(cfg)))
    regexparam = re.compile(r'(\{[^\v\}]*\})')
    result = content

    # start matching all variables
    m = regexvar.search(result)
    while m is not None:
        var = m.group(1)
        varlow = var.lower()

        val = ''

        start = m.start()
        end = m.end()
        # check if we have a function for the variable
        if varlow in VAR_FUNCS and make_boolean_evaluable is not True:
            param = {}
            if result[m.end()] == '{':
                m2 = regexparam.search(result, m.end())
                if m2 is not None:
                    param = json.loads(m2.group(1))
                    end = m2.end()
            val = VAR_FUNCS[varlow](cfg, filepath, param)
        elif varlow in VAR_USER:
            if make_boolean_evaluable:
                val = ' True ' if VAR_USER[varlow] else ' False '
            else:
                val = VAR_USER[varlow]
        else:
            if make_boolean_evaluable:
                val = ' False '
            else:
                print('Warning: unknown variable {}'.format(var))
                val = '<unknown>'

        result = result[:start] + val + result[end:]
        m = regexvar.search(result, m.start())

    return result


def extract_vars_from_templates(cfg):
    esc = get_esc(cfg)
    regex_all_lists = re.compile(r'{0}{0}foreach\s*\(\s*{0}(\w+){0}?\s*\)\s*^.*?{0}{0}endfor\s?'.format(esc), re.MULTILINE | re.DOTALL)
    regex_all_strings = re.compile(r'["\'][\S ]*?(?<!{0}){0}(\w+){0}?[\S ]*["\']'.format(esc))
    regex_all_bools = re.compile(r'{0}{0}if\s*\(\s*{0}(\w+){0}?\s*\)\s*^.*?{0}{0}endif\s?'.format(esc), re.MULTILINE | re.DOTALL)
    regex_all_vars = re.compile(r'{0}(\w+){0}?'.format(esc))

    vars = dict()
    for filepath in cfg.files:
        # check if file exists and read its contents
        if not os.path.exists(filepath):
            print('Warning: template file {} does not exist, skipping'.format(filepath))
            continue

        templ_content = ''
        with open(filepath, 'r') as f:
            templ_content = f.read()

        # Find different variable types and priorize matches in order list > string > bool > unknown ("?")
        # since lists and strings can be evaluated in conditions. Thus, bool is the more general type.
        matches = regex_all_lists.findall(templ_content)
        for match in matches:
            vars[match] = list()

        matches = regex_all_strings.findall(templ_content)
        for match in matches:
            if match not in vars or not isinstance(vars[match], list):
                vars[match] = ''

        matches = regex_all_bools.findall(templ_content)
        for match in matches:
            if match not in vars or (isinstance(vars[match], str) and vars[match] == '?'):
                vars[match] = False

        matches = regex_all_vars.findall(templ_content)
        for match in matches:
            if match not in vars:
                vars[match] = '?'

    # remove keywords and functions
    for k in KEYWORDS:
        if k in vars:
            del vars[k]
    for f in VAR_FUNCS:
        if f in vars:
            del vars[f]

    json_str = json.dumps(vars, sort_keys=True, indent=4)
    print(json_str)


def create_templates(cfg):
    for filepath in cfg.files:
        filepath = os.path.abspath(filepath)
        filename = os.path.basename(filepath)
        ext = os.path.splitext(filename)[1]
        if ext[0] == '.':
            ext = ext[1:]

        if ext not in cfg.map_ext:
            print("Warning: no template for extension .{}".format(ext))
            continue

        if not cfg.force and os.path.exists(filepath):
            print("Warning: {} file exists, skipping".format(filename))
            continue

        templ_name = cfg.map_ext[ext]
        templ_path = ''

        # find correct dir for the matching template file
        for templ_dir in cfg.search_dir:
            templ_dir = os.path.abspath(templ_dir)
            templ_path = os.path.join(templ_dir, templ_name)
            if os.path.exists(templ_path):
                break

        # check if we found a suitable template path
        if not os.path.exists(templ_path):
            print('Warning: no path found for {}'.format(templ_name))
            continue

        # read the content from the template file
        templ_content = ''
        with open(templ_path, 'r') as f:
            templ_content = f.read()

        # replace if...else conditions
        templ_content = replace_conditions(templ_content, cfg, filepath)

        # replace foreach loops
        templ_content = replace_loops(templ_content, cfg, filepath)

        # replace variables given in template
        templ_content = replace_vars(templ_content, cfg, filepath)

        # write the new file
        with open(filepath, 'w') as f:
            f.write(templ_content)

        print('[OK] {}'.format(filename))


if __name__ == '__main__':
    cfg = parse_arguments()

    if cfg.extract_vars_json:
        extract_vars_from_templates(cfg)
    else:
        create_templates(cfg)
