## BIN = camerademo

LIB_SEARCH_PATHS := \
	./libs \

INCLUDES := \
    $(CUR_PATH) \
	-I${LIBMAIX_PATH}/components/libmaix/include \
	-I${LIBQUIRC_PATH} \
	-I/media/ricardo/Dados/Outros/Linux/IPCAM/YI_HOME/yi-hack-Allwinner/TESTES/alsa-lib/include 

LIBS := libcdc_base \
        libglog \
        libion \
        libisp_ini \
        libISP \
        liblog \
        libmaix_cam \
        libmaix_nn \
        libmaix_image \
        libmaix_utils \
        libmedia_utils \
        libMemAdapter \
        libmpp_isp \
        libmpp_mini \
        libmpp_vi \
        libVE \
        libasound \

STATIC_LIBS :=  liblibmaix \
                libquirc

#CFLAGS += -fpermissive


SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := .
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)


LDFLAGS := $(LDFLAGS) \
    $(patsubst %,-L%,$(LIB_SEARCH_PATHS)) \
    -Wl,-rpath-link=$(subst $(space),:,$(strip $(LIB_SEARCH_PATHS))) \
    -Wl,-Bstatic \
    -Wl,--start-group $(foreach n, $(STATIC_LIBS), -l$(patsubst lib%,%,$(patsubst %.a,%,$(notdir $(n))))) -Wl,--end-group \
    -Wl,-Bdynamic \
    $(foreach y, $(LIBS), -l$(patsubst lib%,%,$(patsubst %.so,%,$(notdir $(y)))))


$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BIN): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

all:$(BIN)

$(OBJ_DIR):
	mkdir -p $@

clean:
	@$(RM) -rv $(BIN) $(OBJ_DIR) # The @ disables the echoing of the command
