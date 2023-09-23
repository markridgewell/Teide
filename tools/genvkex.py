#!/usr/bin/python3
#
# Copyright 2013-2023 The Khronos Group Inc.
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
import pdb
import re
import sys
import copy
import xml.etree.ElementTree as etree

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
    parser.add_argument('-emitSpirv', action='append',
                        default=[],
                        help='Specify a SPIR-V extension or capability to emit in targets')
    parser.add_argument('-emitFormats', action='append',
                        default=[],
                        help='Specify Vulkan Formats to emit in targets')
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
    parser.add_argument('-genpath', action='store', default='gen',
                        help='Path to generated files')
    parser.add_argument('-o', action='store', dest='directory',
                        default='.',
                        help='Create target and related files in specified directory')
    parser.add_argument('target', metavar='target', nargs='?',
                        help='Specify target')
    parser.add_argument('-quiet', action='store_true', default=True,
                        help='Suppress script output during normal execution.')
    parser.add_argument('-verbose', action='store_false', dest='quiet', default=True,
                        help='Enable script output during normal execution.')
    parser.add_argument('--vulkanLayer', action='store_true', dest='vulkanLayer',
                        help='Enable scripts to generate VK specific vulkan_json_data.hpp for json_gen_layer.')
    parser.add_argument('-misracstyle', dest='misracstyle', action='store_true',
                        help='generate MISRA C-friendly headers')
    parser.add_argument('-misracppstyle', dest='misracppstyle', action='store_true',
                        help='generate MISRA C++-friendly headers')
    parser.add_argument('--iscts', action='store_true', dest='isCTS',
                        help='Specify if this should generate CTS compatible code')

    return parser.parse_args()


sys.path.append(os.path.abspath(os.path.dirname(get_args().registry)))

print(os.path.abspath(os.path.dirname(get_args().registry)))

from vkexgenerator import VkexGeneratorOptions, VkexOutputGenerator
from cgenerator import CGeneratorOptions, COutputGenerator
from generator import write
from reg import Registry
from apiconventions import APIConventions


def makeREstring(strings, default=None, strings_are_regex=False):
    """Turn a list of strings into a regexp string matching exactly those strings."""
    if strings or default is None:
        if not strings_are_regex:
            strings = (re.escape(s) for s in strings)
        return '^(' + '|'.join(strings) + ')$'
    return default


def makeGenOpts(args):
    """Returns a directory of [ generator function, generator options ] indexed
    by specified short names. The generator options incorporate the following
    parameters:

    args is an parsed argument object; see below for the fields that are used."""
    global genOpts
    genOpts = {}

    # Default class of extensions to include, or None
    defaultExtensions = APIConventions

    # Additional extensions to include (list of extensions)
    extensions = args.extension

    # Extensions to remove (list of extensions)
    removeExtensions = args.removeExtensions

    # Extensions to emit (list of extensions)
    emitExtensions = args.emitExtensions

    # SPIR-V capabilities / features to emit (list of extensions & capabilities)
    emitSpirv = args.emitSpirv

    # Vulkan Formats to emit
    emitFormats = args.emitFormats

    # Features to include (list of features)
    features = args.feature

    # Whether to disable inclusion protect in headers
    protect = args.protect

    # Output target directory
    directory = args.directory

    # Path to generated files, particularly apimap.py
    genpath = args.genpath

    # Generate MISRA C-friendly headers
    misracstyle = args.misracstyle;

    # Generate MISRA C++-friendly headers
    misracppstyle = args.misracppstyle;

    # Descriptive names for various regexp patterns used to select
    # versions and extensions
    allFormats = allSpirv = allFeatures = allExtensions = r'.*'

    # Turn lists of names/patterns into matching regular expressions
    addExtensionsPat     = makeREstring(extensions, None)
    removeExtensionsPat  = makeREstring(removeExtensions, None)
    emitExtensionsPat    = makeREstring(emitExtensions, allExtensions)
    emitSpirvPat         = makeREstring(emitSpirv, allSpirv)
    emitFormatsPat       = makeREstring(emitFormats, allFormats)
    featuresPat          = makeREstring(features, allFeatures)

    # Copyright text prefixing all headers (list of strings).
    # The SPDX formatting below works around constraints of the 'reuse' tool
    prefixStrings = [
        '/*',
        '** Copyright 2015-2023 The Khronos Group Inc.',
        '**',
        '** SPDX-License-Identifier' + ': Apache-2.0',
        '*/',
        ''
    ]

    # Text specific to Vulkan headers
    vkPrefixStrings = [
        '/*',
        '** This header is generated from the Khronos Vulkan XML API Registry.',
        '**',
        '*/',
        ''
    ]

    vulkanLayer = args.vulkanLayer

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

    isCTS = args.isCTS

    # Platform extensions, in their own header files
    # Each element of the platforms[] array defines information for
    # generating a single platform:
    #   [0] is the generated header file name
    #   [1] is the set of platform extensions to generate
    #   [2] is additional extensions whose interfaces should be considered,
    #   but suppressed in the output, to avoid duplicate definitions of
    #   dependent types like VkDisplayKHR and VkSurfaceKHR which come from
    #   non-platform extensions.

    # Track all platform extensions, for exclusion from vulkan_core.h
    allPlatformExtensions = []

    # Extensions suppressed for all WSI platforms (WSI extensions required
    # by all platforms)
    commonSuppressExtensions = [ 'VK_KHR_display', 'VK_KHR_swapchain' ]

    # Extensions required and suppressed for beta "platform". This can
    # probably eventually be derived from the requires= attributes of
    # the extension blocks.
    betaRequireExtensions = [
        'VK_KHR_portability_subset',
        'VK_KHR_video_encode_queue',
        'VK_EXT_video_encode_h264',
        'VK_EXT_video_encode_h265',
        'VK_NV_displacement_micromap',
        'VK_AMDX_shader_enqueue',
    ]

    betaSuppressExtensions = [
        'VK_KHR_video_queue',
        'VK_EXT_opacity_micromap',
        'VK_KHR_pipeline_library',
    ]

    platforms = [
        [ 'vulkan_android.h',     [ 'VK_KHR_android_surface',
                                    'VK_ANDROID_external_memory_android_hardware_buffer'
                                                                  ], commonSuppressExtensions +
                                                                     [ 'VK_KHR_format_feature_flags2',
                                                                     ] ],
        [ 'vulkan_fuchsia.h',     [ 'VK_FUCHSIA_imagepipe_surface',
                                    'VK_FUCHSIA_external_memory',
                                    'VK_FUCHSIA_external_semaphore',
                                    'VK_FUCHSIA_buffer_collection' ], commonSuppressExtensions ],
        [ 'vulkan_ggp.h',         [ 'VK_GGP_stream_descriptor_surface',
                                    'VK_GGP_frame_token'          ], commonSuppressExtensions ],
        [ 'vulkan_ios.h',         [ 'VK_MVK_ios_surface'          ], commonSuppressExtensions ],
        [ 'vulkan_macos.h',       [ 'VK_MVK_macos_surface'        ], commonSuppressExtensions ],
        [ 'vulkan_vi.h',          [ 'VK_NN_vi_surface'            ], commonSuppressExtensions ],
        [ 'vulkan_wayland.h',     [ 'VK_KHR_wayland_surface'      ], commonSuppressExtensions ],
        [ 'vulkan_win32.h',       [ 'VK_.*_win32(|_.*)', 'VK_.*_winrt(|_.*)', 'VK_EXT_full_screen_exclusive' ],
                                                                     commonSuppressExtensions +
                                                                     [ 'VK_KHR_external_semaphore',
                                                                       'VK_KHR_external_memory_capabilities',
                                                                       'VK_KHR_external_fence',
                                                                       'VK_KHR_external_fence_capabilities',
                                                                       'VK_KHR_get_surface_capabilities2',
                                                                       'VK_NV_external_memory_capabilities',
                                                                     ] ],
        [ 'vulkan_xcb.h',         [ 'VK_KHR_xcb_surface'          ], commonSuppressExtensions ],
        [ 'vulkan_xlib.h',        [ 'VK_KHR_xlib_surface'         ], commonSuppressExtensions ],
        [ 'vulkan_directfb.h',    [ 'VK_EXT_directfb_surface'     ], commonSuppressExtensions ],
        [ 'vulkan_xlib_xrandr.h', [ 'VK_EXT_acquire_xlib_display' ], commonSuppressExtensions ],
        [ 'vulkan_metal.h',       [ 'VK_EXT_metal_surface',
                                    'VK_EXT_metal_objects'        ], commonSuppressExtensions ],
        [ 'vulkan_screen.h',      [ 'VK_QNX_screen_surface',
                                    'VK_QNX_external_memory_screen_buffer' ], commonSuppressExtensions ],
        [ 'vulkan_sci.h',         [ 'VK_NV_external_sci_sync',
                                    'VK_NV_external_sci_sync2',
                                    'VK_NV_external_memory_sci_buf'], commonSuppressExtensions ],
        [ 'vulkan_beta.h',        betaRequireExtensions,             betaSuppressExtensions ],
    ]

    for platform in platforms:
        headername = platform[0]

        allPlatformExtensions += platform[1]

        addPlatformExtensionsRE = makeREstring(
            platform[1] + platform[2], strings_are_regex=True)
        emitPlatformExtensionsRE = makeREstring(
            platform[1], strings_are_regex=True)

        opts = CGeneratorOptions(
            conventions       = conventions,
            filename          = headername,
            directory         = directory,
            genpath           = None,
            apiname           = defaultAPIName,
            mergeApiNames     = mergeApiNames,
            profile           = None,
            versions          = featuresPat,
            emitversions      = None,
            defaultExtensions = None,
            addExtensions     = addPlatformExtensionsRE,
            removeExtensions  = None,
            emitExtensions    = emitPlatformExtensionsRE,
            prefixText        = prefixStrings + vkPrefixStrings,
            genFuncPointers   = True,
            protectFile       = protectFile,
            protectFeature    = False,
            protectProto      = '#ifndef',
            protectProtoStr   = 'VK_NO_PROTOTYPES',
            apicall           = 'VKAPI_ATTR ',
            apientry          = 'VKAPI_CALL ',
            apientryp         = 'VKAPI_PTR *',
            alignFuncParam    = 48,
            misracstyle       = misracstyle,
            misracppstyle     = misracppstyle)

        genOpts[headername] = [ COutputGenerator, opts ]

    # Header for core API + extensions.
    # To generate just the core API,
    # change to 'defaultExtensions = None' below.
    #
    # By default this adds all enabled, non-platform extensions.
    # It removes all platform extensions (from the platform headers options
    # constructed above) as well as any explicitly specified removals.

    removeExtensionsPat = makeREstring(
        allPlatformExtensions + removeExtensions, None, strings_are_regex=True)

    genOpts['vkex.hpp'] = [
          VkexOutputGenerator,
          VkexGeneratorOptions(
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
            prefixText        = prefixStrings + vkPrefixStrings,
            genFuncPointers   = True,
            protectFile       = protectFile,
            protectFeature    = False,
            protectProto      = '#ifndef',
            protectProtoStr   = 'VK_NO_PROTOTYPES',
            apicall           = 'VKAPI_ATTR ',
            apientry          = 'VKAPI_CALL ',
            apientryp         = 'VKAPI_PTR *',
            alignFuncParam    = 48,
            misracstyle       = misracstyle,
            misracppstyle     = misracppstyle)
        ]

    genOpts['vulkan_core.h'] = [
          COutputGenerator,
          CGeneratorOptions(
            conventions       = conventions,
            filename          = 'vulkan_core.h',
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
            prefixText        = prefixStrings + vkPrefixStrings,
            genFuncPointers   = True,
            protectFile       = protectFile,
            protectFeature    = False,
            protectProto      = '#ifndef',
            protectProtoStr   = 'VK_NO_PROTOTYPES',
            apicall           = 'VKAPI_ATTR ',
            apientry          = 'VKAPI_CALL ',
            apientryp         = 'VKAPI_PTR *',
            alignFuncParam    = 48,
            misracstyle       = misracstyle,
            misracppstyle     = misracppstyle)
        ]

    # Vulkan versions to include for SC header - SC *removes* features from 1.0/1.1/1.2
    scVersions = makeREstring(['VK_VERSION_1_0', 'VK_VERSION_1_1', 'VK_VERSION_1_2', 'VKSC_VERSION_1_0'])

    genOpts['vulkan_sc_core.h'] = [
          COutputGenerator,
          CGeneratorOptions(
            conventions       = conventions,
            filename          = 'vulkan_sc_core.h',
            directory         = directory,
            apiname           = 'vulkansc',
            profile           = None,
            versions          = scVersions,
            emitversions      = scVersions,
            defaultExtensions = 'vulkansc',
            addExtensions     = addExtensionsPat,
            removeExtensions  = removeExtensionsPat,
            emitExtensions    = emitExtensionsPat,
            prefixText        = prefixStrings + vkPrefixStrings,
            genFuncPointers   = True,
            protectFile       = protectFile,
            protectFeature    = False,
            protectProto      = '#ifndef',
            protectProtoStr   = 'VK_NO_PROTOTYPES',
            apicall           = 'VKAPI_ATTR ',
            apientry          = 'VKAPI_CALL ',
            apientryp         = 'VKAPI_PTR *',
            alignFuncParam    = 48,
            misracstyle       = misracstyle,
            misracppstyle     = misracppstyle)
        ]

    genOpts['vulkan_sc_core.hpp'] = [
          COutputGenerator,
          CGeneratorOptions(
            conventions       = conventions,
            filename          = 'vulkan_sc_core.hpp',
            directory         = directory,
            apiname           = 'vulkansc',
            profile           = None,
            versions          = scVersions,
            emitversions      = scVersions,
            defaultExtensions = 'vulkansc',
            addExtensions     = addExtensionsPat,
            removeExtensions  = removeExtensionsPat,
            emitExtensions    = emitExtensionsPat,
            prefixText        = prefixStrings + vkPrefixStrings,
            genFuncPointers   = True,
            protectFile       = protectFile,
            protectFeature    = False,
            protectProto      = '#ifndef',
            protectProtoStr   = 'VK_NO_PROTOTYPES',
            apicall           = 'VKAPI_ATTR ',
            apientry          = 'VKAPI_CALL ',
            apientryp         = 'VKAPI_PTR *',
            alignFuncParam    = 48,
            misracstyle       = misracstyle,
            misracppstyle     = misracppstyle)
        ]

    # keep any relevant platform extensions for the following generators
    # (needed for e.g. the vulkan_sci extensions)
    explicitRemoveExtensionsPat = makeREstring(
        removeExtensions, None, strings_are_regex=True)

    # Unused - vulkan10.h target.
    # It is possible to generate a header with just the Vulkan 1.0 +
    # extension interfaces defined, but since the promoted KHR extensions
    # are now defined in terms of the 1.1 interfaces, such a header is very
    # similar to vulkan_core.h.
    genOpts['vulkan10.h'] = [
          COutputGenerator,
          CGeneratorOptions(
            conventions       = conventions,
            filename          = 'vulkan10.h',
            directory         = directory,
            genpath           = None,
            apiname           = defaultAPIName,
            profile           = None,
            versions          = 'VK_VERSION_1_0',
            emitversions      = 'VK_VERSION_1_0',
            defaultExtensions = None,
            addExtensions     = None,
            removeExtensions  = None,
            emitExtensions    = None,
            prefixText        = prefixStrings + vkPrefixStrings,
            genFuncPointers   = True,
            protectFile       = protectFile,
            protectFeature    = False,
            protectProto      = '#ifndef',
            protectProtoStr   = 'VK_NO_PROTOTYPES',
            apicall           = 'VKAPI_ATTR ',
            apientry          = 'VKAPI_CALL ',
            apientryp         = 'VKAPI_PTR *',
            alignFuncParam    = 48,
            misracstyle       = misracstyle,
            misracppstyle     = misracppstyle)
        ]

    # Video header target - combines all video extension dependencies into a
    # single header, at present.
    genOpts['vk_video.h'] = [
          COutputGenerator,
          CGeneratorOptions(
            conventions       = conventions,
            filename          = 'vk_video.h',
            directory         = directory,
            genpath           = None,
            apiname           = 'vulkan',
            profile           = None,
            versions          = None,
            emitversions      = None,
            defaultExtensions = defaultExtensions,
            addExtensions     = addExtensionsPat,
            removeExtensions  = removeExtensionsPat,
            emitExtensions    = emitExtensionsPat,
            prefixText        = prefixStrings + vkPrefixStrings,
            genFuncPointers   = True,
            protectFile       = protectFile,
            protectFeature    = False,
            protectProto      = '#ifndef',
            protectProtoStr   = 'VK_NO_PROTOTYPES',
            apicall           = '',
            apientry          = '',
            apientryp         = '',
            alignFuncParam    = 48,
            misracstyle       = misracstyle,
            misracppstyle     = misracppstyle)
    ]

    # Video extension 'Std' interfaces, each in its own header files
    # These are not Vulkan extensions, or a part of the Vulkan API at all.
    # They are treated in a similar fashion for generation purposes, but
    # all required APIs for each interface must be explicitly required.
    #
    # Each element of the videoStd[] array is an extension name defining an
    # interface, and is also the basis for the generated header file name.

    videoStd = [
        'vulkan_video_codecs_common',
        'vulkan_video_codec_h264std',
        'vulkan_video_codec_h264std_decode',
        'vulkan_video_codec_h264std_encode',
        'vulkan_video_codec_h265std',
        'vulkan_video_codec_h265std_decode',
        'vulkan_video_codec_h265std_encode',
    ]

    # Unused at present
    # addExtensionRE = makeREstring(videoStd)
    for codec in videoStd:
        headername = f'{codec}.h'

        # Consider all of the codecs 'extensions', but only emit this one
        emitExtensionRE = makeREstring([codec])

        opts = CGeneratorOptions(
            conventions       = conventions,
            filename          = headername,
            directory         = directory,
            genpath           = None,
            apiname           = defaultAPIName,
            mergeApiNames     = mergeApiNames,
            profile           = None,
            versions          = None,
            emitversions      = None,
            defaultExtensions = None,
            addExtensions     = emitExtensionRE,
            removeExtensions  = None,
            emitExtensions    = emitExtensionRE,
            #requireDepends    = False,
            prefixText        = prefixStrings + vkPrefixStrings,
            genFuncPointers   = False,
            protectFile       = protectFile,
            protectFeature    = False,
            alignFuncParam    = 48,
            )

        genOpts[headername] = [ COutputGenerator, opts ]

    # Unused - vulkan11.h target.
    # It is possible to generate a header with just the Vulkan 1.0 +
    # extension interfaces defined, but since the promoted KHR extensions
    # are now defined in terms of the 1.1 interfaces, such a header is very
    # similar to vulkan_core.h.
    genOpts['vulkan11.h'] = [
          COutputGenerator,
          CGeneratorOptions(
            conventions       = conventions,
            filename          = 'vulkan11.h',
            directory         = directory,
            genpath           = None,
            apiname           = defaultAPIName,
            profile           = None,
            versions          = '^VK_VERSION_1_[01]$',
            emitversions      = '^VK_VERSION_1_[01]$',
            defaultExtensions = None,
            addExtensions     = None,
            removeExtensions  = None,
            emitExtensions    = None,
            prefixText        = prefixStrings + vkPrefixStrings,
            genFuncPointers   = True,
            protectFile       = protectFile,
            protectFeature    = False,
            protectProto      = '#ifndef',
            protectProtoStr   = 'VK_NO_PROTOTYPES',
            apicall           = 'VKAPI_ATTR ',
            apientry          = 'VKAPI_CALL ',
            apientryp         = 'VKAPI_PTR *',
            alignFuncParam    = 48,
            misracstyle       = misracstyle,
            misracppstyle     = misracppstyle)
        ]

    genOpts['alias.h'] = [
          COutputGenerator,
          CGeneratorOptions(
            conventions       = conventions,
            filename          = 'alias.h',
            directory         = directory,
            genpath           = None,
            apiname           = defaultAPIName,
            profile           = None,
            versions          = featuresPat,
            emitversions      = featuresPat,
            defaultExtensions = defaultExtensions,
            addExtensions     = None,
            removeExtensions  = removeExtensionsPat,
            emitExtensions    = emitExtensionsPat,
            prefixText        = None,
            genFuncPointers   = False,
            protectFile       = False,
            protectFeature    = False,
            protectProto      = '',
            protectProtoStr   = '',
            apicall           = '',
            apientry          = '',
            apientryp         = '',
            alignFuncParam    = 36)
        ]


def genTarget(args):
    """Create an API generator and corresponding generator options based on
    the requested target and command line options.

    This is encapsulated in a function so it can be profiled and/or timed.
    The args parameter is an parsed argument object containing the following
    fields that are used:

    - target - target to generate
    - directory - directory to generate it in
    - protect - True if re-inclusion wrappers should be created
    - extensions - list of additional extensions to include in generated interfaces"""

    # Create generator options with parameters specified on command line
    makeGenOpts(args)

    # Select a generator matching the requested target
    if args.target in genOpts:
        createGenerator = genOpts[args.target][0]
        options = genOpts[args.target][1]

        gen = createGenerator(errFile=errWarn,
                              warnFile=errWarn,
                              diagFile=diag)
        return (gen, options)
    else:
        print('No generator options for unknown target:', args.target)
        return None


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
