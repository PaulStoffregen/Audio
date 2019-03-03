#!/usr/bin/env bash
# define colors
GRAY='\033[1;30m'; RED='\033[0;31m'; LRED='\033[1;31m'; GREEN='\033[0;32m'; LGREEN='\033[1;32m'; ORANGE='\033[0;33m'; YELLOW='\033[1;33m'; BLUE='\033[0;34m'; LBLUE='\033[1;34m'; PURPLE='\033[0;35m'; LPURPLE='\033[1;35m'; CYAN='\033[0;36m'; LCYAN='\033[1;36m'; LGRAY='\033[0;37m'; WHITE='\033[1;37m';
examples=($(find $HOME/build -name "*.pde" -o -name "*.ino"))
for example in "${examples[@]}"; do
  echo -n $example:
  $HOME/arduino_ide/arduino-$ARDUINO_IDE_VERSION/arduino --verify --board "teensy:avr:teensy36:usb=serial,speed=180,opt=o2std,keys=en-us" $example 2> error.txt > output.txt
  local platform_switch=${PIPESTATUS[0]}
  # notify if the platform switch failed
  if [ $platform_switch -ne 0 ]; then
    # heavy X
    echo -e """$RED""\xe2\x9c\x96"
    echo -e "\n"
    cat error.txt
    exit_code=1
  else
    # heavy checkmark
    echo -e """$GREEN""\xe2\x9c\x93"
    #cat output.txt
  fi
done;
exit $exit_code
