# Fonction appelée lors du premier context switch d'un processus afin d'appeler
# proc_launcher (qui passe au mode U et lance le processus) avec le code du processus.

    .globl init_proc_launcher
    .text
init_proc_launcher:
    mv a0, s0
    j proc_launcher # proc_launcher ne termine jamais
