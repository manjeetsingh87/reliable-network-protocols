#!/bin/bash
       
#Title           :assignment2_update_verifier.sh
#description     :This script will update the experiment scripts for assignment2.
#Author		     :Swetank Kumar Saha <swetankk@buffalo.edu>
#Version         :1.0
#====================================================================================

cd scripts
wget -r --no-parent -nH --cut-dirs=3 -R index.html https://ubwins.cse.buffalo.edu/cse-489_589/scripts/
cd ..