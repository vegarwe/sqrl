
ssh vegarwe@localhost "(cd /home/vegarwe/link_devel/nRF5_SDK_15.3.0_59ac345/examples/crypto/sqrl/tomu-sqrl; make)"

/c/bin/im-tomu/dfu-util --reset --download tomu-sqrl.dfu
