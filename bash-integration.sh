# Alias Manager integration for interactive Bash.

[[ $- == *i* ]] || return 0

# Do not register the integration twice in the same shell.
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
    local executable="$HOME/.local/bin/alias-manager"
    local definitions name
    local -A before=()

    [[ -x "$executable" ]] || return "$previous_status"

    # If export fails, keep the current aliases unchanged.
    if ! definitions=$("$executable" export); then
        return "$previous_status"
    fi

    # No changes in storage: leave the shell untouched.
    if [[ $__am_initialized == 1 &&
          $definitions == "$__am_last_export" ]]; then
        return "$previous_status"
    fi

    # Remove our previous definitions only if they are still unchanged.
    # Restore any aliases that existed before we replaced them.
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

    # Remember the shell state before loading the new definitions.
    for name in "${!BASH_ALIASES[@]}"; do
        before["$name"]=${BASH_ALIASES[$name]}
    done

    # The text comes from our program's Bash-quoted export.
    if ! builtin source /dev/stdin <<< "$definitions"; then
        printf '%s\n' "alias-manager: cannot load exported aliases" >&2
        return "$previous_status"
    fi

    # Track only definitions that we added or changed.
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

# Preserve existing handlers, including VS Code's handler.
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