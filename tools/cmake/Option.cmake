macro(
    option_feature
    name
    feature_name
    description
    default)
    option(${name} "${description}" ${default})
    if(${name})
        list(APPEND VCPKG_MANIFEST_FEATURES "${feature_name}")
    endif()
endmacro()

macro(option_enum name description values)
    list(GET values 0 first_value)
    set(${name}
        "${first_value}"
        CACHE STRING "${description}")
    set_property(CACHE ${name} PROPERTY STRINGS "${values}")
endmacro()
