#include <iostream>
__attribute__ ((naked)) void print_poop() {
  asm
  (R"(
      .intel_syntax
      mov eax, 1            # syscall number for write
      mov edi, 1            # file descriptor for stdout
      lea rsi, [rip + str]  # string to print
      mov rdx, 5            # number of chars
      syscall               # call write
      ret                   # return from function
 
      str:         
        .ascii "Poop\n"
  )");
}

__attribute__ ((naked)) int reboob() {
  asm
  (R"(
      .intel_syntax
      mov eax, 169         # syscall number for reboot
      mov edi, 0xfee1dead  # magic1
      mov esi, 672274793   # magic2
      mov edx, 0x4321fedc  # LINUX_REBOOT_CMD_POWER_OFF value
      lea r10, [rip + goodbye_message] # shutdown message
      
      syscall # call reboot
      ret     # return (no you won't)

      goodbye_message:
        .ascii "Goodbye cruel world"
  )");
}

int main() {
    print_poop();

    if (reboob())
        printf ("Failed to hax0r system");
}