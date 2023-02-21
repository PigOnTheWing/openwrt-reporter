include $(TOPDIR)/rules.mk

# Name, version and release number
# The name and version of your package are used to define the variable to point to the build directory of your package: $(PKG_BUILD_DIR)
PKG_NAME:=reporter
PKG_VERSION:=1.0
PKG_RELEASE:=1

# Source settings (i.e. where to find the source codes)
# This is a custom variable, used below
SOURCE_DIR:=/your/src/dir

include $(INCLUDE_DIR)/package.mk

# Package definition; instructs on how and where our package will appear in the overall configuration menu ('make menuconfig')
define Package/reporter
  SECTION:=custom
  CATEGORY:=Custom
  DEPENDS:=+libubus +libuci +libmosquitto-ssl
  TITLE:=Reporter
endef

TARGET_CFLAGS += -I$(STAGING_DIR)/usr/include
TARGET_LDFLAGS += -luci -lubus -lubox -lmosquitto

# Package description; a more verbose description on what our package does
define Package/reporter/description
	An application reporting some params to a MQTT server
endef

# Package preparation instructions; create the build directory and copy the source code. 
# The last command is necessary to ensure our preparation instructions remain compatible with the patching system.
define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	cp $(SOURCE_DIR)/*.{c,h} $(PKG_BUILD_DIR)
	cp $(SOURCE_DIR)/Makefile $(PKG_BUILD_DIR)
	cp $(SOURCE_DIR)/pkg/reporter_conf $(PKG_BUILD_DIR)
	$(Build/Patch)
endef

# Package build instructions; invoke the target-specific compiler to first compile the source file, and then to link the file into the final executable
define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
  	CC="$(TARGET_CC)" \
    CFLAGS="$(TARGET_CFLAGS)" \
    LDFLAGS="$(TARGET_LDFLAGS)"
endef

# Package install instructions; create a directory inside the package to hold our executable, and then copy the executable we built previously into the folder
define Package/reporter/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/reporter $(1)/usr/bin
	$(INSTALL_CONF) $(PKG_BUILD_DIR)/reporter_conf $(1)/etc/config
endef

$(info $(TARGET_LDFLAGS))

# This command is always the last, it uses the definitions and variables we give above in order to get the job done
$(eval $(call BuildPackage,reporter))
