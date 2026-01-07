https://wiki.osdev.org/EDK2
built without platforms

git clone https://github.com/tianocore/edk2.git
cd edk2
git submodule update --init

export EDK_TOOLS_PATH=$PWD/BaseTools
source edksetup.sh
make -C BaseTools

mkdir BootPartuuid
vim BootPartuuid/BootPartuuid.inf
vim BootPartuuid/UefiMain.c

build -a X86 -t GCC5 -p MdeModulePkg/MdeModulePkg.dsc

sudo modprobe nbd max_part=8

sudo qemu-nbd --connect=/dev/nbd0 /var/lib/libvirt/images/uefitest-1.qcow2
sudo mount /dev/nbd0p1 /mnt/usb

cp Build/MdeModule/DEBUG_GCC5/X86/BootPartuuid.efi <...>

sudo umount /mnt/usb
sudo qemu-nbd --disconnect /dev/nbd0


made branch partuuid
