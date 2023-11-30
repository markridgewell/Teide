#!/usr/bin/python3

import argparse
import os
import pdb
import re
import sys
import xml.etree.ElementTree as etree

from vkexgenerator import VkexGeneratorOptions, VkexOutputGenerator
from reg import Registry
from apiconventions import APIConventions


def get_args():
    parser = argparse.ArgumentParser()

    parser.add_argument('-apiname', action='store',
                        default=None,
                        help='Specify API to generate (defaults to repository-specific conventions object value)')
    parser.add_argument('-mergeApiNames', action='store',
                        default=None,
                        help='Specify a comma separated list of APIs to merge into the target API')
    parser.add_argument('-extension', action='append',
                        default=[],
                        help='Specify an extension or extensions to add to targets')
    parser.add_argument('-removeExtensions', action='append',
                        default=[],
                        help='Specify an extension or extensions to remove from targets')
    parser.add_argument('-emitExtensions', action='append',
                        default=[],
                        help='Specify an extension or extensions to emit in targets')
    parser.add_argument('-feature', action='append',
                        default=[],
                        help='Specify a core API feature name or names to add to targets')
    parser.add_argument('-debug', action='store_true',
                        help='Enable debugging')
    parser.add_argument('-dump', action='store_true',
                        help='Enable dump to stderr')
    parser.add_argument('-diagfile', action='store',
                        default=None,
                        help='Write diagnostics to specified file')
    parser.add_argument('-errfile', action='store',
                        default=None,
                        help='Write errors and warnings to specified file instead of stderr')
    parser.add_argument('-noprotect', dest='protect', action='store_false',
                        help='Disable inclusion protection in output headers')
    parser.add_argument('-profile', action='store_true',
                        help='Enable profiling')
    parser.add_argument('-registry', action='store',
                        required=True,
                        help='Use specified registry file')
    parser.add_argument('-o', action='store', dest='directory',
                        default='.',
                        help='Create target and related files in specified directory')
    parser.add_argument('-quiet', action='store_true', default=True,
                        help='Suppress script output during normal execution.')
    parser.add_argument('-verbose', action='store_false', dest='quiet', default=True,
                        help='Enable script output during normal execution.')
    parser.add_argument('-misracstyle', dest='misracstyle', action='store_true',
                        help='generate MISRA C-friendly headers')
    parser.add_argument('-misracppstyle', dest='misracppstyle', action='store_true',
                        help='generate MISRA C++-friendly headers')

    return parser.parse_args()

def makeREstring(strings, default=None, strings_are_regex=False):
    """Turn a list of strings into a regexp string matching exactly those strings."""
    if strings or default is None:
        if not strings_are_regex:
            strings = (re.escape(s) for s in strings)
        return '^(' + '|'.join(strings) + ')$'
    return default


def genTarget(args):
    """Returns a directory of [ generator function, generator options ] indexed
    by specified short names. The generator options incorporate the following
    parameters:

    args is an parsed argument object; see below for the fields that are used."""
    genOpts = {}

    # Default class of extensions to include, or None
    defaultExtensions = APIConventions

    # Additional extensions to include (list of extensions)
    extensions = args.extension

    # Extensions to remove (list of extensions)
    removeExtensions = args.removeExtensions

    # Extensions to emit (list of extensions)
    emitExtensions = args.emitExtensions

    # Features to include (list of features)
    features = args.feature

    # Whether to disable inclusion protect in headers
    protect = args.protect

    # Output target directory
    directory = args.directory

    # Generate MISRA C-friendly headers
    misracstyle = args.misracstyle;

    # Generate MISRA C++-friendly headers
    misracppstyle = args.misracppstyle;

    # Descriptive names for various regexp patterns used to select
    # versions and extensions
    allFeatures = allExtensions = r'.*'

    # Turn lists of names/patterns into matching regular expressions
    addExtensionsPat     = r'.*'#makeREstring(extensions, None)
    removeExtensionsPat  = makeREstring(removeExtensions, None)
    emitExtensionsPat    = makeREstring(emitExtensions, allExtensions)
    featuresPat          = makeREstring(features, allFeatures)

    # Text specific to Vulkan headers
    prefixStrings = [
        '/*',
        '** This header is generated from the Khronos Vulkan XML API Registry.',
        '**',
        '*/',
        ''
    ]

    # Defaults for generating re-inclusion protection wrappers (or not)
    protectFile = protect

    # An API style conventions object
    conventions = APIConventions()

    if args.apiname is not None:
        defaultAPIName = args.apiname
    else:
        defaultAPIName = conventions.xml_api_name

    # APIs to merge
    mergeApiNames = args.mergeApiNames

    removeExtensionsPat = makeREstring(
        removeExtensions, None, strings_are_regex=True)

    os.makedirs(directory, exist_ok=True)

    options = VkexGeneratorOptions(
        conventions       = conventions,
        filename          = 'vkex.hpp',
        directory         = directory,
        genpath           = None,
        apiname           = defaultAPIName,
        mergeApiNames     = mergeApiNames,
        profile           = None,
        versions          = featuresPat,
        emitversions      = featuresPat,
        defaultExtensions = defaultExtensions,
        addExtensions     = addExtensionsPat,
        removeExtensions  = removeExtensionsPat,
        emitExtensions    = emitExtensionsPat,
        prefixText        = prefixStrings,
        genFuncPointers   = False,
        protectFile       = protectFile,
        protectFeature    = True,
        protectProto      = '#ifndef',
        protectProtoStr   = 'VK_NO_PROTOTYPES',
        apicall           = 'VKAPI_ATTR ',
        apientry          = 'VKAPI_CALL ',
        apientryp         = 'VKAPI_PTR *',
        alignFuncParam    = 48,
        misracstyle       = misracstyle,
        misracppstyle     = misracppstyle)

    gen = VkexOutputGenerator(errFile=errWarn,
                              warnFile=errWarn,
                              diagFile=diag)
    return (gen, options)


# -feature name
# -extension name
# For both, "name" may be a single name, or a space-separated list
# of names, or a regular expression.
if __name__ == '__main__':
    args = get_args()

    # This splits arguments which are space-separated lists
    args.feature = [name for arg in args.feature for name in arg.split()]
    args.extension = [name for arg in args.extension for name in arg.split()]

    # create error/warning & diagnostic files
    if args.errfile:
        errWarn = open(args.errfile, 'w', encoding='utf-8')
    else:
        errWarn = sys.stderr

    if args.diagfile:
        diag = open(args.diagfile, 'w', encoding='utf-8')
    else:
        diag = None

    # Create the API generator & generator options
    (gen, options) = genTarget(args)

    # Create the registry object with the specified generator and generator
    # options. The options are set before XML loading as they may affect it.
    reg = Registry(gen, options)

    # Parse the specified registry XML into an ElementTree object
    tree = etree.parse(args.registry)

    # Load the XML tree into the registry object
    reg.loadElementTree(tree)

    if args.dump:
        print('* Dumping registry to regdump.txt')
        reg.dumpReg(filehandle=open('regdump.txt', 'w', encoding='utf-8'))

    # Finally, use the output generator to create the requested target
    if args.debug:
        pdb.run('reg.apiGen()')
    else:
        reg.apiGen()

   # if not args.quiet:
        print('* Generated', options.filename)
