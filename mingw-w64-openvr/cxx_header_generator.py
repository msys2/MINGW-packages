#! python3

import argparse
import shlex
import sys
import os
import re
import io

parser = argparse.ArgumentParser(
                prog='cxx_header_generator', description="Generate C++ header for openvr",
                epilog='Run cxx_header_generator.py --help for more information',
                allow_abbrev=False)

parser.add_argument("--header-dir", "-d", dest="headdir", help="directory to openvr headers", required=True)
args = parser.parse_args()

input_path = args.headdir

if not os.path.isdir(input_path):
    print('The path specified does not exist')
    sys.exit()

cHeader = os.path.join(input_path, 'openvr.h')
cppHeader = os.path.join(input_path, 'openvr_mingw.hpp')
header = open(cHeader, newline='\n').read()

annoyingMacroPattern = re.compile(r'#define\s+(\w+).*VR_CLANG_ATTR.*')
fullClassPattern = re.compile(r'^([^\S\n]*)class\s+(\w+).*?\{.*?\};', re.MULTILINE | re.DOTALL)
virtualPattern = re.compile(r'([^\S\n]*)virtual\s(.*?)(\w+)(\((.+?\s*(sizeof\s*\(\s*.*?\s*\)\s*)?,\s*)*.*?\s*(sizeof\s*\(\s*.*?\s*\)\s*)?\))\s*=\s*0\s*;[^\S\n]*', re.MULTILINE)
optionalParamPattern = re.compile(r'\s*=\s*(sizeof\s*\(\s*.*?\s*\)\s*)?[^,)]+')
versionPattern = re.compile(r'\s*static\s*const\s*char\s*\*\s*const\s*IVR\w+\s*=\s*\"IVR\w+\";')

annoyingMacros = [match.group(1) for match in annoyingMacroPattern.finditer(header)]

newHeader = header
for match in versionPattern.finditer(newHeader):
    newHeader = newHeader.replace(match.group(0), match.group(0).replace('"IVR', '"FnTable:IVR'))

for match in fullClassPattern.finditer(header):
    if match.group(0).find('virtual') == -1:
        continue
    fullClass = match.group(0)
    indent = match.group(1)
    className = match.group(2)
    
    newClass = fullClass
    
    declarations = []
    for function in virtualPattern.finditer(fullClass):
        returnType = function.group(2)
        functionName = function.group(3)
        params = function.group(4)
        paramsNoDefault = re.sub(optionalParamPattern, '', params)
        for macro in annoyingMacros:
            paramsNoDefault = re.sub(macro + r'\(.*?\)', '', paramsNoDefault)
        
        args = []
        paramTokens = list(shlex.shlex(io.StringIO(paramsNoDefault)))
        for i, token in enumerate(paramTokens):
            if token == ',' or (token == ')' and paramTokens[i - 1] not in ['(', 'void']):
                args.append(paramTokens[i - 1])
        
        declaration = indent + '\t{}(__stdcall *{}){};'.format(returnType, functionName, paramsNoDefault)
        definition = indent + '\t' + returnType + functionName + params + ' { '
        if returnType.strip() != 'void':
            definition = definition + 'return '
        definition = definition + '_table.' + functionName + '(' + ', '.join(args) + '); }'
        
        declarations.append(declaration)
        newClass = newClass.replace(function.group(0), definition)
        
    tableName = 'VR_{}_FnTable'.format(className)
    table = indent + 'struct ' + tableName + '\n' + indent + '{\n' + '\n'.join(declarations) + '\n' + indent + '};\n\n'
    
    newClass = newClass.replace('public:', '\t{} _table;\n{}public:'.format(tableName, indent))
    
    newHeader = newHeader.replace(fullClass, table + newClass)

open(cppHeader, 'w', newline='\n').write(newHeader)
