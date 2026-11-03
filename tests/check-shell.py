from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
for relative in ['bash-integration.sh', 'packaging/alias-manager', 'packaging/session', 'scripts/build-deb.sh']:
    subprocess.run(['bash', '-n', str(root / relative)], check=True)
with tempfile.TemporaryDirectory() as temporary:
    directory = Path(temporary)
    arguments = directory / 'arguments'
    environment = directory / 'environment'
    for name, expected in [
        ('alacritty', ['--title', 'Alias Manager', '-e', '/usr/lib/alias-manager/session']),
        ('ptyxis', ['--standalone', '--new-window', '--title=Alias Manager', '--', '/usr/lib/alias-manager/session']),
        ('kitty', ['--title', 'Alias Manager', '/usr/lib/alias-manager/session'])
    ]:
        mock = directory / name
        mock.write_text('#!/bin/bash\nprintf "%s\\0" "$@" > "$AM_TEST_ARGS"\nenv > "$AM_TEST_ENV"\n')
        mock.chmod(0o755)
        env = os.environ.copy()
        env.update(PATH=str(directory) + ':/usr/bin:/bin', DISPLAY=':99',
                   ALIAS_MANAGER_TERMINAL=name, AM_TEST_ARGS=str(arguments),
                   AM_TEST_ENV=str(environment), GTK_PATH='/snap/code/current/gtk',
                   GSETTINGS_SCHEMA_DIR='/home/test/snap/code/schemas',
                   XDG_DATA_DIRS='/snap/core20/usr/share')
        subprocess.run(['bash', str(root / 'packaging/alias-manager')], env=env, check=True)
        actual = arguments.read_bytes().rstrip(b'\0').decode().split('\0')
        assert actual == expected, (name, actual)
        saved = environment.read_text().splitlines()
        assert not any(line.startswith('GTK_PATH=') for line in saved)
        assert not any(line.startswith('GSETTINGS_SCHEMA_DIR=') for line in saved)
        assert 'XDG_CONFIG_DIRS=/etc/xdg' in saved
    backend = directory / 'backend'
    definitions = directory / 'definitions'
    backend.write_text('#!/bin/bash\ncat -- "$AM_TEST_DATA"\n')
    backend.chmod(0o755)
    definitions.write_text("alias am_test='echo initial'\n")
    env = os.environ.copy()
    env.update(HOME=temporary, AM_TEST_DATA=str(definitions),
               AM_TEST_HOOK=str(root / 'bash-integration.sh'),
               ALIAS_MANAGER_BACKEND=str(backend))
    script = r'''
PROMPT_COMMAND=__vsc_prompt_cmd_original
source "$AM_TEST_HOOK"
[[ ${BASH_ALIASES[am_test]} == 'echo initial' ]] || exit 10
printf "alias am_test='echo updated'\n" > "$AM_TEST_DATA"
__am_sync
[[ ${BASH_ALIASES[am_test]} == 'echo updated' ]] || exit 11
: > "$AM_TEST_DATA"
__am_sync
[[ ! ${BASH_ALIASES[am_test]+exists} ]] || exit 12
alias am_restore='echo original'
printf "alias am_restore='echo managed'\n" > "$AM_TEST_DATA"
__am_sync
[[ ${BASH_ALIASES[am_restore]} == 'echo managed' ]] || exit 13
: > "$AM_TEST_DATA"
__am_sync
[[ ${BASH_ALIASES[am_restore]} == 'echo original' ]] || exit 14
[[ ${PROMPT_COMMAND[0]} == '__vsc_prompt_cmd_original' ]] || exit 15
false
__am_sync
[[ $? == 1 ]] || exit 16
'''
    subprocess.run(['bash', '--noprofile', '--norc', '-ic', script], env=env, check=True, capture_output=True)
print('Shell syntax, launcher argument/environment, and Bash integration checks passed.')
