+++ Build +++
ssh vegarwe@localhost "(cd ~/link_devel/nRF5_SDK_15.3.0_59ac345/examples/crypto/sqrl/nrf_sqrl/pca10059/mbr/armgcc; make -j8)"


+++ Flash +++
cd C:\Users\vegar.westerlund\devel\nRF5_SDK_15.3.0_59ac345\examples\peripheral\cli\pca10059
nrfutil pkg generate --hw-version 52 --sd-req 0x00 --debug-mode --application mbr\arm5_no_packs\_build\nrf52840_xxaa.hex dfu.zip
nrfutil dfu usb-serial -pkg dfu.zip -p COM4 -b 115200

nrfutil pkg generate --hw-version 52 --sd-req 0x00 --debug-mode --application pca10059/mbr/armgcc/_build/nrf52840_xxaa.hex dfu.zip


+++ Test ++++
python -u sqrl_comm_test.py com9

