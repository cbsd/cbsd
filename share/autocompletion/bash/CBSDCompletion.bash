_cbsd () {
    # Remove = from COMP_WORDBREAKS because it is used when jname= commands
    COMP_WORDBREAKS=${COMP_WORDBREAKS/\=/}
    local cur
    # cur: Current word where cursor is located
    cur=${COMP_WORDS[COMP_CWORD]}

    # We only get CBSD subcomand list if there is not already one defined
    if [ ${#COMP_WORDS[@]} -eq 2 ]; then
        # Show only the options that start with $cur
        COMPREPLY=( $( compgen -W "$( cbsd help nodesc|sed -r '/^\s*$/d'|grep -v ' ' )" -- $cur ) )
        return 0
    # If we have the subcommand, we check for arguments only for some common used commands
    elif [ ${#COMP_WORDS[@]} -eq 3 ]; then
        case ${COMP_WORDS[1]} in
            bconfig)
                VMS=$(env NOCOLOR=1 cbsd bls display=jname header=0|awk '{print"jname="$1}'|tr -s '\n' ' ')
                COMPREPLY=( $( compgen -W "$VMS" -- $cur ) )
                ;;
            blogin)
                VMS=""
                N=$(env NOCOLOR=1 cbsd bls display=jname header=0|wc -l|awk '{print$1}')
                for I in $(seq 1 $N); do
                    STATE=$(env NOCOLOR=1 cbsd bls display=status header=0|tail -n $I|head -n 1)
                    if [ "$STATE" == "On" ]; then
                        VM=$(env NOCOLOR=1 cbsd bls display=jname header=0|tail -n $I|head -n 1)
                        if [ "$VMS" == "" ]; then
                            VMS=$VM
                        else
                            VMS="$VMS $VM"
                        fi
                    fi
                done
                COMPREPLY=( $( compgen -W "$VMS" -- $cur ) )
                ;;
            bstart)
                VMS=""
                N=$(env NOCOLOR=1 cbsd bls display=jname header=0|wc -l|awk '{print$1}')
                for I in $(seq 1 $N); do
                    STATE=$(env NOCOLOR=1 cbsd bls display=status header=0|tail -n $I|head -n 1)
                    if [ "$STATE" == "Off" ]; then
                        VM=$(env NOCOLOR=1 cbsd bls display=jname header=0|tail -n $I|head -n 1)
                        if [ "$VMS" == "" ]; then
                            VMS=$VM
                        else
                            VMS="$VMS $VM"
                        fi
                    fi
                done
                COMPREPLY=( $( compgen -W "$VMS" -- $cur ) )
                ;;
            bstop)
                VMS=""
                N=$(env NOCOLOR=1 cbsd bls display=jname header=0|wc -l|awk '{print$1}')
                for I in $(seq 1 $N); do
                    STATE=$(env NOCOLOR=1 cbsd bls display=status header=0|tail -n $I|head -n 1)
                    if [ "$STATE" == "On" ]; then
                        VM=$(env NOCOLOR=1 cbsd bls display=jname header=0|tail -n $I|head -n 1)
                        if [ "$VMS" == "" ]; then
                            VMS=$VM
                        else
                            VMS="$VMS $VM"
                        fi
                    fi
                done
                COMPREPLY=( $( compgen -W "$VMS" -- $cur ) )
                ;;
            bremove)
                VMS=$(env NOCOLOR=1 cbsd bls display=jname header=0|tr -s '\n' ' ')
                COMPREPLY=( $( compgen -W "$VMS" -- $cur ) )
                ;;
            jconfig)
                JAILS=$(env NOCOLOR=1 cbsd jls display=jname header=0|awk '{print"jname="$1}'|tr -s '\n' ' ')
                COMPREPLY=( $( compgen -W "$JAILS" -- $cur ) )
                ;;
            jlogin)
                JAILS=""
                N=$(env NOCOLOR=1 cbsd jls display=jname header=0|wc -l|awk '{print$1}')
                for I in $(seq 1 $N); do
                        STATE=$(env NOCOLOR=1 cbsd jls display=status header=0|tail -n $I|head -n 1)
                        if [ "$STATE" == "On" ]; then
                            JAIL=$(env NOCOLOR=1 cbsd jls display=jname header=0|tail -n $I|head -n 1)
                            if [ "$JAILS" == "" ]; then
                                JAILS=$JAIL
                            else
                                JAILS="$JAILS $JAIL"
                            fi
                        fi
                done
                COMPREPLY=( $( compgen -W "$JAILS" -- $cur ) )
                ;;
            jstart)
                JAILS=""
                N=$(env NOCOLOR=1 cbsd jls display=jname header=0|wc -l|awk '{print$1}')
                for I in $(seq 1 $N); do
                        STATE=$(env NOCOLOR=1 cbsd jls display=status header=0|tail -n $I|head -n 1)
                        if [ "$STATE" == "Off" ]; then
                            JAIL=$(env NOCOLOR=1 cbsd jls display=jname header=0|tail -n $I|head -n 1)
                            if [ "$JAILS" == "" ]; then
                                JAILS=$JAIL
                            else
                                JAILS="$JAILS $JAIL"
                            fi
                        fi
                done
                COMPREPLY=( $( compgen -W "$JAILS" -- $cur ) )
                ;;
            jstop)
                JAILS=""
                N=$(env NOCOLOR=1 cbsd jls display=jname header=0|wc -l|awk '{print$1}')
                for I in $(seq 1 $N); do
                        STATE=$(env NOCOLOR=1 cbsd jls display=status header=0|tail -n $I|head -n 1)
                        if [ "$STATE" == "On" ]; then
                            JAIL=$(env NOCOLOR=1 cbsd jls display=jname header=0|tail -n $I|head -n 1)
                            if [ "$JAILS" == "" ]; then
                                JAILS=$JAIL
                            else
                                JAILS="$JAILS $JAIL"
                            fi
                        fi
                done
                COMPREPLY=( $( compgen -W "$JAILS" -- $cur ) )
                ;;
            jremove)
                JAILS=$(env NOCOLOR=1 cbsd jls display=jname header=0|tr -s '\n' ' ')
                COMPREPLY=( $( compgen -W "$JAILS" -- $cur ) )
                ;;
            *)
                return 0
                ;;
        esac
    else
        return 0
    fi
}

complete -F _cbsd cbsd
