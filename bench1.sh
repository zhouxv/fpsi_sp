# #! /bin/bash
# set -e

# # Cleanup function to handle script termination
# # This function will be called on script exit or interruption
# cleanup() {
#     pkill -P $$  # Kill all the child processes of the current process group
#     # Optional: Delete temporary files
#     [ -f "$TMP_FILE" ] && rm "$TMP_FILE"
#     exit 1
# }

# # Register Signal Capture
# trap 'cleanup' INT TERM EXIT

# ns=(8 12 16)
# dims=(4 8 16)
# deltas=(16 32)

# printf "[ProType] [Metric] [Dim] [Delta] [Size] [Com.(MB)] [Time(s)]\n"

# for n in "${ns[@]}"; do
#   for dim in "${dims[@]}"; do
#     for delta in "${deltas[@]}"; do
#       ./build/fpsi -d $dim -delta $delta -nn $n -p 0 -try 1
#       ./build/fpsi -d $dim -delta $delta -nn $n -p 1 -try 1 
#       ./build/fpsi -d $dim -delta $delta -nn $n -p 2 -try 1 
#       echo 
#     done
#   done
# done



sudo apt update && \
sudo apt install -y zsh git curl && \
sh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)" "" --unattended && \
git clone https://github.com/zsh-users/zsh-syntax-highlighting.git ${ZSH_CUSTOM:-~/.oh-my-zsh/custom}/plugins/zsh-syntax-highlighting && \
git clone https://github.com/zsh-users/zsh-history-substring-search ${ZSH_CUSTOM:-~/.oh-my-zsh/custom}/plugins/zsh-history-substring-search && \
sed -i 's/plugins=(git)/plugins=(git zsh-history-substring-search zsh-syntax-highlighting)/' ~/.zshrc && \
chsh -s $(which zsh) && \
source ~/.zshrc