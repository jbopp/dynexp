# codetempl

A command line utility to generate code files from templates written in python.

## Install

Use pip to install ```codetempl``` directly from GitHub.

```
pip install git+https://github.com/Rookfighter/code-template.git
```

## Usage

After the installation the command ```codetempl``` should be available. The most
important options are described in the following table.

| Parameter                           | Description                                   |
|-------------------------------------|-----------------------------------------------|
| ```--search-dir <dir>```          | Add a search directory where ```codetempl``` looks for template files. |
| ```--map-ext <ext:template-file>``` | Add an extension to template file mapping. ```codetempl``` selects a template file depending on the extension of the file that will be created. ```codetempl``` searches in the defined search directories for the template file. |
| ```--esc-char <char>```           | Escape character for template variables and control blocks (default ```$```). |
| ```--config <cfg-file>```         | Load command line parameters from the specified file. ```codetempl``` will also automatically look for a ```.codetemplrc``` in your home directory. |
| ```-v```, ```--version```         | Shows version number. |
| ```-f```, ```--force```           | Force overwrite for existing files. Otherwise existing files will be skipped. |
| ```-e <files>```, ```--extract-vars-json <files>``` | Print variables occurring in the specified template files in JSON format. Also attempts to determine the variables' types (list ```[]```, string ```""```, boolean ```false```, unknown ```"?"```). Parameters related to code generation will be ignored if this flag is specified. |
| ```--user-var <var:value>```     | Define value for a variable. The variable ```var``` will be replaced with the content of ```value```. |
| ```--vars-json <json-file>```    | Load variables from a file in JSON format. This parameter might be combined with the ```--user-var``` parameter. |

A config file might look like this:

```
search-dir /home/user/Templates
map-ext py:template.py
map-ext cpp:template.cpp
map-ext hpp:template.hpp
map-ext c:template.c
map-ext h:template.h
```

In this case if a new .cpp file is created ```codetempl``` will look in
```/home/user/Templates``` for a template file called ```template.cpp```.

### Template variables

There are multiple predefined variables which can be specified in a template
file with a leading ```$```. These will be replaced on creation of a new file.
It is also possible to define variables as command line argument. The names of
variables start with the character after the leading ```$``` and include every
subsequent character in the set ```[a-zA-Z0-9_]``` until a character not included
in this set is reached. ```$``` can be used to terminate a veriable name
(especially if the variable occurs within a string, e.g. ```A$FOO$String``` where
the variable ```FOO``` might be replaced). The leading as well as the optional
terminating ```$``` will be removed upon replacement. Variable names are not case-
sensitive!

| Variable       | Description                       | Options                 |
|----------------|-----------------------------------|-------------------------|
| ```date```     | Current date.  | ```fmt``` specify format of the date (Python date formats). Default = ```dd mmm yyyy```. |
| ```user```     | Current system user name.          | - |
| ```gituser```  | Current git ```user.name```. Takes local repo configuration into consideration. | - |
| ```gitemail``` | Current git ```user.email```. Takes local repo configuration into consideration. | - |
| ```filename``` | Filename of currently created file. | - |
| ```filepath``` | Absolute filepath of currently created file. | - |
| ```guard```    | C/C++ style header guard. | ```lvl``` Defines how many parent dirs should be part of the guard. Default = 0. |

The options are passed as a JSON object appended to the variable name.
A template file for a C header file might look like this:

```c
/* $filename
 *
 * Created on: $date
 *     Author: $gituser
 */

#ifndef $guard{"lvl":1}
#define $guard{"lvl":1}

#endif
```

For more examples on template files see the ```templates``` directory.

### Loops
Lists assigned to variables (like ```"Libs": ["foo.lib", "bar.lib"]``` in a
JSON file) might be expanded using foreach loops. A template

```c
$$foreach($Libs)
#pragma comment(lib, "Lib/$Libs")
$$endfor
```

will result in a form where the block within the foreach loop is reproduced
for every element of the list given in parentheses after the ```$$foreach```
statement:

```c
#pragma comment(lib, "Lib/foo.lib")
#pragma comment(lib, "Lib/bar.lib")
```

### Conditions
Code blocks in template files might be embedded in condition blocks to make them
appear in the files created from that templates only if certain conditions are met
or are *not* met. The syntax works as follows:

```
$$if(CONDITION)
[Code to be reproduced only if CONDITION is met]
$$else
[Code to be reproduced only if CONDITION is NOT met]
$$endif
```

All variables appearing in *CONDITION* will be replaced by a boolean value by casting
their definitions into a boolean type. Noteworthy, empty lists and empty strings will
evaluate to ```False``` according to the Python rules. Variables which do not exist
will be replaced by ```False``` as well. After all replacements, the final condition
will be evaluated by Python's ```eval()``` function. Doing so, Python's logical
operators are allowed in the condition statement to construct more complex conditions
by combining multiple variables.

#### Example
Given the following variable definitions as a JSON file
```json
{
  "Libs": [],
  "Flag": true,
}
```
the following template
```c
$$if($Libs and $Flag)
std::cout << "Libs is not empty and Flag is true."
$$else
std::cout << "Libs is empty or Flag is false."
$$endif
```
will be evaluated to
```c
std::cout << "Libs is empty or Flag is false."
```

### Evaluation order
Conditions are evaluated firstly, loops secondly and normal variables thirdly. Thus,
it is possible to use normal variables within loops and to use normal variables as
well as loops within condition blocks.

## Example

You can create a new C++ class with the sample template files by executing
the following:

```
codetempl --user-var "class:MyClass" --user-var "namespace:MyNamespace" MyClass.hpp MyClass.cpp
```
