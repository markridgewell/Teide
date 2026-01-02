# Remove a single argument from an argument string
#
# call as : remove_arg(command "-Werror")
macro(remove_arg command_var arg)
    string(REPLACE " ${arg} " " " tmp_var " ${${command_var}} ")
    string(STRIP "${tmp_var}" tmp_var)
    set(${command_var}
        ${tmp_var}
        PARENT_SCOPE)
endmacro()
