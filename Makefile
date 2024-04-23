#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

all: st_final.rel st_village.rel st_otrain.rel st_starfox.rel st_stadium.rel \
    sora_adv_menu_save_load.rel sora_adv_menu_save_point.rel sora_adv_menu_seal.rel \
    sora_adv_menu_sel_char.rel sora_menu_sel_char_access.rel demos

demos: st_demo.rel

st_final.rel:
	@cd st_final && make

st_demo.rel:
	@cd demos/st_demo && make

st_village.rel:
	@cd st_village && make

st_otrain.rel:
	@cd st_otrain && make

st_starfox.rel:
	@cd st_starfox && make

st_stadium.rel:
	@cd st_stadium && make

sora_adv_menu_save_load.rel:
	@cd sora_adv_menu_save_load && make

sora_adv_menu_save_point.rel:
	@cd sora_adv_menu_save_point && make

sora_adv_menu_seal.rel:
	@cd sora_adv_menu_seal && make

sora_adv_menu_sel_char.rel:
	@cd sora_adv_menu_sel_char && make

sora_menu_sel_char_access.rel:
	@cd sora_menu_sel_char_access && make

clean:
	@cd st_final && make clean
	@cd st_village && make clean
	@cd demos/st_demo && make clean
	@cd st_otrain && make clean
	@cd st_starfox && make clean
	@cd st_stadium && make clean
	@cd sora_adv_menu_save_load && make clean
	@cd sora_adv_menu_save_point && make clean
	@cd sora_adv_menu_seal && make clean
	@cd sora_adv_menu_sel_char && make clean
	@cd sora_menu_sel_char_access && make clean
