cbsd
====

Yet one more wrapper around FreeBSD jail.
For more information please visit website: http://www.bsdstore.ru


Autocompletion sample for bash:
--
_cbsd () {
        local cur
        cur=${COMP_WORDS[COMP_CWORD]}
        COMPREPLY=( $( compgen -W '$( cbsd help nodesc )' -- $cur ) )
        return 0
}

complete -F _cbsd cbsd
--
