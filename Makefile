CXXFLAGS += -I$(USERFS_PUB)include -std=c++1y -I. -MD -MP -I/home/bubu/Desktop/jushu/R16/alexa-event-v2/pub/R16/include
#-I./include -I/home/bubu/Desktop/jushu/R16/alexa-event-v2/pub/R16/include -I/home/bubu/Desktop/jushu/R16/alexa-event-v2/src/alexa-realizer/alsa/include/
LDFLAGS += -L$(PROJ_DIR)/src/alexa-realizer/alsa/lib -lasound -L$(PROJ_DIR)/src/alexa-realizer/asr/ -lmw -lUsMicArray
LDFLAGS += -L$(USERFS_PUB)/lib  -L$(USERFS_PUB)/local/lib -L$(CROSS_LIB_PATH)
LDFLAGS += -lnghttp2_asio -lnghttp2 -lboost_system -lboost_context -lboost_coroutine -lboost_regex -lboost_chrono -lpthread -lboost_thread -pthread -lavformat -lavcodec -lswresample -lavutil -lrt -lm 
LDFLAGS += -lssl -lcrypto -ldl

target_type=module

include $(PROJ_DIR)/project/filemk.tmpl
