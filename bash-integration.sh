
[[ $- == *i* ]] || return 0
[[ ${__am_loaded:-0} == 1 ]] && return 0

declare -g __am_loaded=1
declare -g __am_initialized=0
declare -g __am_last_export=""

declare -gA __am_managed=()
declare -gA __am_original=()
declare -gA __am_had_original=()

__am_sync()
{
    local previous_status=$?
    local executable="${ALIAS_MANAGER_BACKEND:-/usr/lib/alias-manager/alias-manager-core}"
    local definitions name
    local -A before=()

    [[ -x "$executable" ]] || return "$previous_status"
    if ! definitions=$("$executable" export); then
        return "$previous_status"
    fi
    if [[ $__am_initialized == 1 &&
          $definitions == "$__am_last_export" ]]; then
        return "$previous_status"
    fi
    for name in "${!__am_managed[@]}"; do
        if [[ ${BASH_ALIASES[$name]+exists} &&
              ${BASH_ALIASES[$name]} == "${__am_managed[$name]}" ]]; then
            builtin unalias -- "$name"

            if [[ ${__am_had_original[$name]:-0} == 1 ]]; then
                builtin alias -- "$name=${__am_original[$name]}"
            fi
        fi
    done

    __am_managed=()
    __am_original=()
    __am_had_original=()
    for name in "${!BASH_ALIASES[@]}"; do
        before["$name"]=${BASH_ALIASES[$name]}
    done
    if ! builtin source /dev/stdin <<< "$definitions"; then
        printf '%s\n' "alias-manager: cannot load exported aliases" >&2
        return "$previous_status"
    fi
    for name in "${!BASH_ALIASES[@]}"; do
        if [[ ! ${before[$name]+exists} ||
              ${before[$name]} != "${BASH_ALIASES[$name]}" ]]; then
            __am_managed["$name"]=${BASH_ALIASES[$name]}

            if [[ ${before[$name]+exists} ]]; then
                __am_had_original["$name"]=1
                __am_original["$name"]=${before[$name]}
            else
                __am_had_original["$name"]=0
            fi
        fi
    done

    __am_last_export=$definitions
    __am_initialized=1

    return "$previous_status"
}
if [[ $(declare -p PROMPT_COMMAND 2>/dev/null) == "declare -a "* ]]; then
    PROMPT_COMMAND+=(__am_sync)
else
    __am_previous_prompt_command=${PROMPT_COMMAND-}
    PROMPT_COMMAND=()

    if [[ -n $__am_previous_prompt_command ]]; then
        PROMPT_COMMAND+=("$__am_previous_prompt_command")
    fi

    PROMPT_COMMAND+=(__am_sync)
    unset __am_previous_prompt_command
fi
__am_sync
