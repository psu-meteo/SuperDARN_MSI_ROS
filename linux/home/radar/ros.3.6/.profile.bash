# .profile.bash
# =============

export OSTYPE="linux"
export SYSTEM="linux"
export EDITOR="emacs -nw"
export PATH="${PATH}:/opt/local/bin:/usr/bin/:${HOME}/bin:${HOME}/script"

source $RSTPATH/.profile/rst.bash
source $RSTPATH/.profile/base.bash
source $RSTPATH/.profile/general.bash
source $RSTPATH/.profile/superdarn.bash

source $RSTPATH/.profile/idl.bash

