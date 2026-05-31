#include <Arduino.h>
#include "buttons.h"
#include "rotary.h"
#include "ui.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#define MAX_ITEMS 5
#define IMG_W 256
#define IMG_H 256
#define MAX_PACKET_PAYLOAD 8192
#define PACKET_QUEUE_LENGTH 8

static uint32_t last_tick = 0;

struct Packet {
  uint8_t opcode;
  uint16_t length;
  char *payload;
};

static QueueHandle_t packetQueue = NULL;

enum PacketParseState {
  PARSE_OPCODE,
  PARSE_LENGTH_HIGH,
  PARSE_LENGTH_LOW,
  PARSE_PAYLOAD
};

static void freePacket(Packet *pkt) {
  if (!pkt) return;
  if (pkt->payload) free(pkt->payload);
  free(pkt);
}

static Packet *allocatePacket(uint8_t opcode, uint16_t length) {
  Packet *pkt = (Packet *)malloc(sizeof(Packet));
  if (!pkt) return NULL;

  pkt->opcode = opcode;
  pkt->length = length;
  pkt->payload = NULL;

  if (length > 0) {
    pkt->payload = (char *)malloc(length + 1);
    if (!pkt->payload) {
      free(pkt);
      return NULL;
    }
    pkt->payload[length] = '\0';
  }

  return pkt;
}

static bool enqueuePacket(Packet *pkt) {
  if (!pkt || !packetQueue) {
    freePacket(pkt);
    return false;
  }

  if (xQueueSend(packetQueue, &pkt, 0) != pdTRUE) {
    freePacket(pkt);
    return false;
  }

  return true;
}

static void packetReaderTask(void *pvParameters) {
  PacketParseState state = PARSE_OPCODE;
  uint8_t currentOpcode = 0;
  uint16_t currentLength = 0;
  uint16_t bytesRead = 0;
  Packet *pending = NULL;
  uint32_t lastByteTime = millis();

  while (true) {
    if (Serial.available() > 0) {
      lastByteTime = millis();
      int incoming = Serial.read();
      if (incoming < 0) {
        continue;
      }

      switch (state) {
        case PARSE_OPCODE:
          currentOpcode = (uint8_t)incoming;
          state = PARSE_LENGTH_HIGH;
          break;

        case PARSE_LENGTH_HIGH:
          currentLength = ((uint16_t)incoming) << 8;
          state = PARSE_LENGTH_LOW;
          break;

        case PARSE_LENGTH_LOW:
          currentLength |= (uint16_t)incoming;
          if (currentLength > MAX_PACKET_PAYLOAD) {
            state = PARSE_OPCODE;
            currentLength = 0;
            break;
          }
          pending = allocatePacket(currentOpcode, currentLength);
          if (!pending) {
            state = PARSE_OPCODE;
            currentLength = 0;
            break;
          }
          bytesRead = 0;
          if (currentLength == 0) {
            enqueuePacket(pending);
            pending = NULL;
            state = PARSE_OPCODE;
          } else {
            state = PARSE_PAYLOAD;
          }
          break;

        case PARSE_PAYLOAD:
          if (pending && bytesRead < currentLength) {
            pending->payload[bytesRead++] = (char)incoming;
          }
          if (pending && bytesRead >= currentLength) {
            enqueuePacket(pending);
            pending = NULL;
            state = PARSE_OPCODE;
            currentLength = 0;
          }
          break;
      }
    } else {
      if (state != PARSE_OPCODE && (millis() - lastByteTime) > 2000) {
        if (pending) {
          freePacket(pending);
          pending = NULL;
        }
        state = PARSE_OPCODE;
        currentLength = 0;
      }
      vTaskDelay(pdMS_TO_TICKS(2));
    }
  }
}

void onRotaryDirection(int8_t direction) {
  if (direction > 0) {
    Serial.print(8);
  } else {
    Serial.print(9);
  }
}

void handleNowPlaying(Packet &pkt) {
  String titles[MAX_ITEMS];
  int nowIndex = 0;
  int start = 0;
  int part = 0;

  int sep = -1;
  for (int i = start; i < pkt.length; i++) {
    if (pkt.payload[i] == '|') {
      sep = i;
      break;
    }
  }
  if (sep >= 0) {
    nowIndex = String(pkt.payload + start, sep - start).toInt();
    start = sep + 1;
  }

  while (part < MAX_ITEMS && start < pkt.length) {
    sep = -1;
    for (int i = start; i < pkt.length; i++) {
      if (pkt.payload[i] == '|') {
        sep = i;
        break;
      }
    }
    if (sep < 0) break;

    // Skip the track index field.
    start = sep + 1;
    if (start >= pkt.length) break;

    int nextSep = -1;
    for (int i = start; i < pkt.length; i++) {
      if (pkt.payload[i] == '|') {
        nextSep = i;
        break;
      }
    }

    if (nextSep < 0) {
      titles[part++] = String(pkt.payload + start, pkt.length - start);
      break;
    }

    titles[part++] = String(pkt.payload + start, nextSep - start);
    start = nextSep + 1;
  }

  if (part > 0) {
    ui_updateNowPlayingTracks(titles, nowIndex);
  }
}

uint8_t *artwork_buf = NULL;
int artwork_index = 0;

void handleArtStart() {
  if (artwork_buf) free(artwork_buf);

  artwork_buf = (uint8_t*)malloc(IMG_W * IMG_H / 8);
  artwork_index = 0;
}
void handleArtChunk(Packet &pkt) {
  for (int i = 0; i < pkt.length; i++) {
    if (artwork_index < (IMG_W * IMG_H / 8)) {
      artwork_buf[artwork_index++] = (uint8_t)pkt.payload[i];
    }
  }
}

void handleArtEnd() {
  ui_updateArtwork(artwork_buf);
}

void handlePacket(Packet &pkt) {
  switch (pkt.opcode) {
    case 0x01: // SET_TITLE
      ui_setText(pkt.payload);
      break;

    case 0x02: // TRACK_START
      break;

    case 0x03: {
      if (pkt.length >= 2) {
        int index = ((uint8_t)pkt.payload[0] << 8) | (uint8_t)pkt.payload[1];
        (void)index;
      }
      break;
    }

    case 0x04: // TRACK_END
      break;

    case 0x05: // ARTWORK_START
      handleArtStart();
      break;

    case 0x06: // ARTWORK_ROW
      handleArtChunk(pkt);
      break;

    case 0x07: // ARTWORK_END
      handleArtEnd();
      break;

    case 0x08:
      handleNowPlaying(pkt);
      break;

    default:
      break;
  }
}

void setup()
{
    Serial.begin(115200);

    packetQueue = xQueueCreate(PACKET_QUEUE_LENGTH, sizeof(Packet *));
    xTaskCreatePinnedToCore(packetReaderTask, "PacketReader", 4096, NULL, 1, NULL, 0);

    pinMode(7, OUTPUT);
    digitalWrite(7, HIGH);

    buttons_init();
    ui_init();
    ui_full_refresh();

    rotary_init();
    rotary_setDirectionHandler(onRotaryDirection);
}
void loop()
{
    Packet *pkt = NULL;
    while (packetQueue && xQueueReceive(packetQueue, &pkt, 0) == pdTRUE) {
      if (pkt) {
        handlePacket(*pkt);
        freePacket(pkt);
      }
    }
    
    buttons_poll();
    ui_render_loop();
    ui_handle_inactivity();

    rotary_update();

    delay(5);
}