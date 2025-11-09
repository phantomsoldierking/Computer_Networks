savedcmd_kernel_firewall.mod := printf '%s\n'   kernel_firewall.o | awk '!x[$$0]++ { print("./"$$0) }' > kernel_firewall.mod
