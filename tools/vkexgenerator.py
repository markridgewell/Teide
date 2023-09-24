
import os
import re

from generator import (GeneratorOptions,
                       MissingGeneratorOptionsConventionsError,
                       MissingGeneratorOptionsError, MissingRegistryError,
                       OutputGenerator, noneStr, regSortFeatures, write)

def concat(a, b):
    if len(a) == 0: return b
    if len(b) == 0: return a
    if (not a[-1].isspace()) and b[0].isalnum():
        return a + ' ' + b
    return a + b

def convertTypeRef(name):
    if name == 'VkBool32':
        return 'bool';
    if name.startswith('Vk'):
        return 'vk::' + name[2:]
    return name

def convertTypeDecl(name):
    if name.startswith('Vk'):
        return name[2:]
    return name

def removePrefix(name, prefix):
    if name.startswith(prefix):
        return name[len(prefix)].lower() + name[len(prefix)+1:]
    return name

def removePointer(decl):
    decl = decl.strip()
    if decl[-1] != '*':
        raise RuntimeError("Expected '*'")
    decl = decl[:-1].strip()
    if decl.endswith('const'):
        return decl[:-5].strip()
    if decl.startswith('const'):
        return decl[5:].strip()
    return decl

class Member:
    def __init__(self, elem):
        self.elem = elem
        self.lengthOf = []
        self.isArray = False

class VkexGeneratorOptions(GeneratorOptions):
    """VkexGeneratorOptions - subclass of GeneratorOptions.

    Adds options used by VkexOutputGenerator objects during C language header
    generation."""

    def __init__(self,
                 prefixText='',
                 genFuncPointers=True,
                 protectFile=True,
                 protectFeature=True,
                 protectProto=None,
                 protectProtoStr=None,
                 protectExtensionProto=None,
                 protectExtensionProtoStr=None,
                 apicall='',
                 apientry='',
                 apientryp='',
                 indentFuncProto=True,
                 indentFuncPointer=False,
                 alignFuncParam=0,
                 genEnumBeginEndRange=False,
                 genAliasMacro=False,
                 genStructExtendsComment=False,
                 aliasMacro='',
                 misracstyle=False,
                 misracppstyle=False,
                 **kwargs
                 ):
        """Constructor.
        Additional parameters beyond parent class:

        - prefixText - list of strings to prefix generated header with
        (usually a copyright statement + calling convention macros)
        - protectFile - True if multiple inclusion protection should be
        generated (based on the filename) around the entire header
        - protectFeature - True if #ifndef..#endif protection should be
        generated around a feature interface in the header file
        - genFuncPointers - True if function pointer typedefs should be
        generated
        - protectProto - If conditional protection should be generated
        around prototype declarations, set to either '#ifdef'
        to require opt-in (#ifdef protectProtoStr) or '#ifndef'
        to require opt-out (#ifndef protectProtoStr). Otherwise
        set to None.
        - protectProtoStr - #ifdef/#ifndef symbol to use around prototype
        declarations, if protectProto is set
        - protectExtensionProto - If conditional protection should be generated
        around extension prototype declarations, set to either '#ifdef'
        to require opt-in (#ifdef protectExtensionProtoStr) or '#ifndef'
        to require opt-out (#ifndef protectExtensionProtoStr). Otherwise
        set to None
        - protectExtensionProtoStr - #ifdef/#ifndef symbol to use around
        extension prototype declarations, if protectExtensionProto is set
        - apicall - string to use for the function declaration prefix,
        such as APICALL on Windows
        - apientry - string to use for the calling convention macro,
        in typedefs, such as APIENTRY
        - apientryp - string to use for the calling convention macro
        in function pointer typedefs, such as APIENTRYP
        - indentFuncProto - True if prototype declarations should put each
        parameter on a separate line
        - indentFuncPointer - True if typedefed function pointers should put each
        parameter on a separate line
        - alignFuncParam - if nonzero and parameters are being put on a
        separate line, align parameter names at the specified column
        - genEnumBeginEndRange - True if BEGIN_RANGE / END_RANGE macros should
        be generated for enumerated types
        - genAliasMacro - True if the OpenXR alias macro should be generated
        for aliased types (unclear what other circumstances this is useful)
        - genStructExtendsComment - True if comments showing the structures
        whose pNext chain a structure extends are included before its
        definition
        - aliasMacro - alias macro to inject when genAliasMacro is True
        - misracstyle - generate MISRA C-friendly headers
        - misracppstyle - generate MISRA C++-friendly headers"""

        GeneratorOptions.__init__(self, **kwargs)

        self.prefixText = prefixText
        """list of strings to prefix generated header with (usually a copyright statement + calling convention macros)."""

        self.genFuncPointers = genFuncPointers
        """True if function pointer typedefs should be generated"""

        self.protectFile = protectFile
        """True if multiple inclusion protection should be generated (based on the filename) around the entire header."""

        self.protectFeature = protectFeature
        """True if #ifndef..#endif protection should be generated around a feature interface in the header file."""

        self.protectProto = protectProto
        """If conditional protection should be generated around prototype declarations, set to either '#ifdef' to require opt-in (#ifdef protectProtoStr) or '#ifndef' to require opt-out (#ifndef protectProtoStr). Otherwise set to None."""

        self.protectProtoStr = protectProtoStr
        """#ifdef/#ifndef symbol to use around prototype declarations, if protectProto is set"""

        self.protectExtensionProto = protectExtensionProto
        """If conditional protection should be generated around extension prototype declarations, set to either '#ifdef' to require opt-in (#ifdef protectExtensionProtoStr) or '#ifndef' to require opt-out (#ifndef protectExtensionProtoStr). Otherwise set to None."""

        self.protectExtensionProtoStr = protectExtensionProtoStr
        """#ifdef/#ifndef symbol to use around extension prototype declarations, if protectExtensionProto is set"""

        self.apicall = apicall
        """string to use for the function declaration prefix, such as APICALL on Windows."""

        self.apientry = apientry
        """string to use for the calling convention macro, in typedefs, such as APIENTRY."""

        self.apientryp = apientryp
        """string to use for the calling convention macro in function pointer typedefs, such as APIENTRYP."""

        self.indentFuncProto = indentFuncProto
        """True if prototype declarations should put each parameter on a separate line"""

        self.indentFuncPointer = indentFuncPointer
        """True if typedefed function pointers should put each parameter on a separate line"""

        self.alignFuncParam = alignFuncParam
        """if nonzero and parameters are being put on a separate line, align parameter names at the specified column"""

        self.genEnumBeginEndRange = genEnumBeginEndRange
        """True if BEGIN_RANGE / END_RANGE macros should be generated for enumerated types"""

        self.genAliasMacro = genAliasMacro
        """True if the OpenXR alias macro should be generated for aliased types (unclear what other circumstances this is useful)"""

        self.genStructExtendsComment = genStructExtendsComment
        """True if comments showing the structures whose pNext chain a structure extends are included before its definition"""

        self.aliasMacro = aliasMacro
        """alias macro to inject when genAliasMacro is True"""

        self.misracstyle = misracstyle
        """generate MISRA C-friendly headers"""

        self.misracppstyle = misracppstyle
        """generate MISRA C++-friendly headers"""

        self.codeGenerator = True
        """True if this generator makes compilable code"""


class VkexOutputGenerator(OutputGenerator):
    """Generates C-language API interfaces."""

    # This is an ordered list of sections in the header file.
    TYPE_SECTIONS = ['include', 'define', 'basetype', 'handle', 'enum',
                     'group', 'bitmask', 'funcpointer', 'struct']
    ALL_SECTIONS = TYPE_SECTIONS + ['commandPointer', 'command']

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        # Internal state - accumulators for different inner block text
        self.sections = {section: [] for section in self.ALL_SECTIONS}
        self.feature_not_empty = False
        self.may_alias = None
        self.emittedStructs = []
        self.structTypeCounts = {'trivial': 0, 'straightforward': 0, 'complex': 0}

    def beginFile(self, genOpts):
        OutputGenerator.beginFile(self, genOpts)
        if self.genOpts is None:
            raise MissingGeneratorOptionsError()
        # C-specific
        #
        # Multiple inclusion protection & C++ wrappers.
        if self.genOpts.protectFile and self.genOpts.filename:
            headerSym = re.sub(r'\.hpp', '_hpp_',
                               os.path.basename(self.genOpts.filename)).upper()
            write('#ifndef', headerSym, file=self.outFile)
            write('#define', headerSym, file=self.outFile)
            self.newline()

        write('#include "vkex_utils.hpp"', file=self.outFile)
        self.newline()

        # User-supplied prefix text, if any (list of strings)
        if genOpts.prefixText:
            for s in genOpts.prefixText:
                write(s, file=self.outFile)

        # C++ extern wrapper - after prefix lines so they can add includes.
        self.newline()
        write('namespace vkex {', file=self.outFile)
        self.newline()

    def endFile(self):
        # C-specific
        # Finish C++ wrapper and multiple inclusion protection
        if self.genOpts is None:
            raise MissingGeneratorOptionsError()
        self.newline()
        write('}', file=self.outFile)
        if self.genOpts.protectFile and self.genOpts.filename:
            self.newline()
            write('#endif', file=self.outFile)

        for type, count in self.structTypeCounts.items():
            print(f'Total {type} structs: {count}')

        # Finish processing in superclass
        OutputGenerator.endFile(self)

    def beginFeature(self, interface, emit):
        # Start processing in superclass
        OutputGenerator.beginFeature(self, interface, emit)
        # C-specific
        # Accumulate includes, defines, types, enums, function pointer typedefs,
        # end function prototypes separately for this feature. They are only
        # printed in endFeature().
        self.sections = {section: [] for section in self.ALL_SECTIONS}
        self.feature_not_empty = False

    def _endProtectComment(self, protect_str, protect_directive='#ifdef'):
        if protect_directive is None or protect_str is None:
            raise RuntimeError('Should not call in here without something to protect')

        # Do not put comments after #endif closing blocks if this is not set
        if not self.genOpts.conventions.protectProtoComment:
            return ''
        elif 'ifdef' in protect_directive:
            return f' /* {protect_str} */'
        else:
            return f' /* !{protect_str} */'

    def endFeature(self):
        "Actually write the interface to the output file."
        # C-specific
        if self.emit:
            if self.feature_not_empty:
                if self.genOpts is None:
                    raise MissingGeneratorOptionsError()
                if self.genOpts.conventions is None:
                    raise MissingGeneratorOptionsConventionsError()
                is_core = self.featureName and self.featureName.startswith(self.conventions.api_prefix + 'VERSION_')
                if self.genOpts.conventions.writeFeature(self.featureExtraProtect, self.genOpts.filename):
                    self.newline()
                    write('#ifdef', self.featureName, file=self.outFile)
                    for section in self.TYPE_SECTIONS:
                        contents = self.sections[section]
                        if contents:
                            write('\n'.join(contents), file=self.outFile)

                    if self.genOpts.genFuncPointers and self.sections['commandPointer']:
                        write('\n'.join(self.sections['commandPointer']), file=self.outFile)
                        self.newline()

                    if self.sections['command']:
                        if self.genOpts.protectProto:
                            write(self.genOpts.protectProto,
                                  self.genOpts.protectProtoStr, file=self.outFile)
                        if self.genOpts.protectExtensionProto and not is_core:
                            write(self.genOpts.protectExtensionProto,
                                  self.genOpts.protectExtensionProtoStr, file=self.outFile)
                        write('\n'.join(self.sections['command']), end='', file=self.outFile)
                        if self.genOpts.protectExtensionProto and not is_core:
                            write('#endif' +
                                  self._endProtectComment(protect_directive=self.genOpts.protectExtensionProto,
                                                          protect_str=self.genOpts.protectExtensionProtoStr),
                                  file=self.outFile)
                        if self.genOpts.protectProto:
                            write('#endif' +
                                  self._endProtectComment(protect_directive=self.genOpts.protectProto,
                                                          protect_str=self.genOpts.protectProtoStr),
                                  file=self.outFile)
                        else:
                            self.newline()

                    write('#endif' +
                        self._endProtectComment(protect_str=self.featureName),
                        file=self.outFile)
        # Finish processing in superclass
        OutputGenerator.endFeature(self)

    def appendSection(self, section, text):
        "Append a definition to the specified section"

        if section is None:
            self.logMsg('error', 'Missing section in appendSection (probably a <type> element missing its \'category\' attribute. Text:', text)
            exit(1)

        self.sections[section].append(text)
        self.feature_not_empty = True

    def genType(self, typeinfo, name, alias):
        "Generate type."
        OutputGenerator.genType(self, typeinfo, name, alias)
        typeElem = typeinfo.elem

        # Vulkan:
        # Determine the category of the type, and the type section to add
        # its definition to.
        # 'funcpointer' is added to the 'struct' section as a workaround for
        # internal issue #877, since structures and function pointer types
        # can have cross-dependencies.
        category = typeElem.get('category')
        if category == 'funcpointer':
            section = 'struct'
        else:
            section = category

        if category in ('struct', 'union'):
            # If the type is a struct type, generate it using the
            # special-purpose generator.
            self.genStruct(typeinfo, name, alias)

    def genProtectString(self, protect_str):
        """Generate protection string.

        Protection strings are the strings defining the OS/Platform/Graphics
        requirements for a given API command.  When generating the
        language header files, we need to make sure the items specific to a
        graphics API or OS platform are properly wrapped in #ifs."""
        protect_if_str = ''
        protect_end_str = ''
        if not protect_str:
            return (protect_if_str, protect_end_str)

        if ',' in protect_str:
            protect_list = protect_str.split(',')
            protect_defs = ('defined(%s)' % d for d in protect_list)
            protect_def_str = ' && '.join(protect_defs)
            protect_if_str = '#if %s\n' % protect_def_str
            protect_end_str = '#endif // %s\n' % protect_def_str
        else:
            protect_if_str = '#ifdef %s\n' % protect_str
            protect_end_str = '#endif // %s\n' % protect_str

        return (protect_if_str, protect_end_str)

    def typeMayAlias(self, typeName):
        if not self.may_alias:
            if self.registry is None:
                raise MissingRegistryError()
            # First time we have asked if a type may alias.
            # So, populate the set of all names of types that may.

            # Everyone with an explicit mayalias="true"
            self.may_alias = set(typeName
                                 for typeName, data in self.registry.typedict.items()
                                 if data.elem.get('mayalias') == 'true')

            # Every type mentioned in some other type's parentstruct attribute.
            polymorphic_bases = (otherType.elem.get('parentstruct')
                                 for otherType in self.registry.typedict.values())
            self.may_alias.update(set(x for x in polymorphic_bases
                                      if x is not None))
        return typeName in self.may_alias

    def elementStr(self, prefix, elem, toArray=False):
        text = noneStr(elem.text).strip()
        tail = noneStr(elem.tail).strip()
        if elem.tag == 'type':
            if False and text in self.emittedStructs:
                text = convertTypeDecl(text)
            else:
                text = convertTypeRef(text)
            if toArray:
                pointerType = concat(concat(prefix, text), tail)
                elemType = removePointer(pointerType)
                if elemType == 'void':
                    elemType = 'std::byte'
                arrayType = f'Array<{elemType}>'
                if not prefix.startswith('const'):
                    arrayType += '*'
                return arrayType
        elif elem.tag == 'name' and toArray:
            text = removePrefix(text, 'p')
        return concat(concat(prefix, text), tail)

    def genStruct(self, typeinfo, typeName, alias):
        """Generate struct (e.g. C "struct" type).

        This is a special case of the <type> tag where the contents are
        interpreted as a set of <member> tags instead of freeform C
        C type declarations. The <member> tags are just like <param>
        tags - they are a declaration of a struct or union member.
        Only simple member declarations are supported (no nested
        structs etc.)

        If alias is not None, then this struct aliases another; just
        generate a typedef of that alias."""
        OutputGenerator.genStruct(self, typeinfo, typeName, alias)

        if self.genOpts is None:
            raise MissingGeneratorOptionsError()

        if alias:
            return

        if typeName not in [
            'VkSubmitInfo'
            #'VkRenderPassCreateInfo',
            #'VkSubmitInfo',
            #'VkPresentInfoKHR'
            ]:
            pass#return

        # List of structs known to work with auto-generation, even though
        # the detection mechanisms aren't able to confirm it.
        autoGenerateableStructs = [
            'VkBufferCreateInfo',
        ]

        structType = 'trivial'

        typeElem = typeinfo.elem

        body = ''
        (protect_begin, protect_end) = self.genProtectString(typeElem.get('protect'))
        if protect_begin:
            body += protect_begin

        if self.genOpts.genStructExtendsComment:
            structextends = typeElem.get('structextends')
            body += '// ' + typeName + ' extends ' + structextends + '\n' if structextends else ''

        body += typeElem.get('category')

        # This is an OpenXR-specific alternative where aliasing refers
        # to an inheritance hierarchy of types rather than C-level type
        # aliases.
        if self.genOpts.genAliasMacro and self.typeMayAlias(typeName):
            body += ' ' + self.genOpts.aliasMacro

        body += ' ' + convertTypeDecl(typeName) + '\n{\n'
        body += '    using MappedType = ' + convertTypeRef(typeName) + ';\n\n'

        members = {}

        for memberElem in typeElem.findall('.//member'):
            nameElem = memberElem.find('name')
            name = nameElem.text
            if name in ['sType', 'pNext']:
                continue
            members[name] = Member(memberElem)
            if memberElem.get('altlen'):
                structType = 'complex'
            if noneStr(nameElem.tail).startswith('['):
                structType = 'complex'
            elif length := memberElem.get('len'):
                if memberElem.get('noautovalidity') and typeName not in autoGenerateableStructs:
                    structType = 'complex'
                isConst = noneStr(memberElem.text).strip().startswith('const')
                if ',' in length:
                    lengthElems = length.split(',')
                    if len(lengthElems) == 2 and lengthElems[1] == 'null-terminated':
                        length = lengthElems[0]
                if length in members:
                    if structType == 'trivial':
                        structType = 'straightforward'
                    members[length].lengthOf += [name]
                    members[name].isArray = True
                    members[name].isConst = isConst
                elif length != 'null-terminated':
                    structType = 'complex'

        # Member declarations
        for member in members.values():
            #body += '// ' + self.makeCParamDecl(member.elem, 0) + ';\n'
            if member.lengthOf:
                continue
            prefix = noneStr(member.elem.text).strip()
            body += '    '
            for elem in member.elem:
                elemStr = self.elementStr(prefix, elem, toArray=member.isArray)
                body = concat(body, elemStr)
                prefix = ''
            body += ' = {};\n'
        body += '\n'

        # Conversion function
        body += '    MappedType map() const\n    {\n'
        for name, member in members.items():
            for m in member.lengthOf[1:]:
                lengthMember = removePrefix(member.lengthOf[0], 'p')
                equalLengthMember = removePrefix(m, 'p')
                if members[m].isConst:
                    body += f'        VKEX_ASSERT({equalLengthMember}.size() == {lengthMember}.size());\n'
                else:
                    body += f'        if ({equalLengthMember} && {equalLengthMember}->size() != {lengthMember}.size())\n'
                    body += '        {\n'
                    body += f'            {equalLengthMember}->reset({lengthMember}.size());\n'
                    body += '        }\n'
        body += '        MappedType r;\n'
        for name, member in members.items():
            value = name
            if member.lengthOf:
                arrayMemberName = removePrefix(member.lengthOf[0], 'p')
                if members[member.lengthOf[0]].isConst:
                    value = f'{arrayMemberName}.size()'
                else:
                    value = f'{arrayMemberName} ? {arrayMemberName}->size() : 0'
            elif member.isArray:
                arrayMemberName = removePrefix(name, 'p')
                if member.isConst:
                    value = f'{arrayMemberName}.data()'
                else:
                    value = f'{arrayMemberName} ? {arrayMemberName}->data() : nullptr'
            body += f'        r.{name} = {value};\n'
        body += '        return r;\n'
        body += '    }\n\n'

        body += '    operator MappedType() const { return map(); }\n'

        body += '};\n'
        if protect_end:
            body += protect_end

        if structType == 'complex':
            body = f'/*\n{body}*/\n'
        #body = f'// {structType} struct\n{body}'
        self.structTypeCounts[structType] += 1

        if structType != 'trivial':
            self.appendSection('struct', body)
            self.emittedStructs += [typeName]

    def misracstyle(self):
        return self.genOpts.misracstyle;

    def misracppstyle(self):
        return self.genOpts.misracppstyle;
