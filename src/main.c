#include "raylib.h"
#include "raymath.h"

#define STB_DS_IMPLEMENTATION
#include "../third-party/stb/stb_ds.h"

#define NOB_IMPLEMENTATION
#include "../nob.h"

#define CARD_WIDTH 200
#define CARD_HEIGHT 350
#define CARD_SPACING 8
#define CARD_SCALE 0.35

#define HEARTS_Y 0
#define DIAMONDS_Y 355
#define SPADES_Y 710
#define CLUBS_Y 1065

#define BACK_STYLE_H_MEANDER_Y 0
#define BACK_STYLE_MEANDER_BORDER_Y 355

typedef enum {
  SUIT_CLUB,
  SUIT_DIAMOND,
  SUIT_HEART,
  SUIT_SPADE,
  SUIT_COUNT
} Suit;

typedef enum {
  VAL_ACE,
  VAL_KING,
  VAL_QUEEN,
  VAL_JACK,
  VAL_TEN,
  VAL_NINE,
  VAL_EIGHT,
  VAL_SEVEN,
  VAL_SIX,
  VAL_FIVE,
  VAL_FOUR,
  VAL_THREE,
  VAL_TWO,
  VAL_COUNT
} Value;

typedef enum {
  BC_RED,
  BC_BLUE,
  BC_YELLOW,
  BC_GREEN,
  BC_GRAY,
  BC_COUNT
} BackColor;

typedef struct {
  Suit suit;
  Value value;
  Rectangle source;
  Vector2 position;
} Card;

typedef struct {
  Card *items;
  size_t capacity;
  size_t count;
} Deck;

typedef struct {
  Rectangle source;
  Vector2 position;
} Back;

typedef struct {
  int key;
  Back value;
} Backs;

typedef struct {
  Deck deck;
  Card *hoveredCard;
  Card *activeCard;
  Backs *backs;
} GameState;

void CreateDeck(Deck *deck) {
  for (int s = SUIT_CLUB; s < SUIT_COUNT; ++s) {
    size_t x = 0;
    size_t y = 0;
    switch (s) {
      case SUIT_CLUB: y = CLUBS_Y; break;
      case SUIT_DIAMOND: y = DIAMONDS_Y; break;
      case SUIT_HEART: y = HEARTS_Y; break;
      case SUIT_SPADE: y = SPADES_Y; break;
    }

    for (int v = VAL_ACE; v < VAL_COUNT; ++v) {
      Rectangle source = { .x = x, .y = y, .width = CARD_WIDTH, .height = CARD_HEIGHT };
      Card c = { .suit = s, .value = v, .source = source, .position = Vector2Zero() };
      nob_da_append(deck, c);
      x += (CARD_WIDTH + CARD_SPACING);
    }
  }
}

void InitPosForTesting(Deck *deck) {
  float x = 20;
  float y = 10;
  for (size_t c = 0; c < deck->count; ++c) {
    if (c > 0 && c % 13 == 0) {
      y += (CARD_HEIGHT*CARD_SCALE) + 10;
      x = 20;
    }
    deck->items[c].position = CLITERAL(Vector2) { x, y };
    x += (CARD_WIDTH*CARD_SCALE)+10;
  }
}

void InitBacksTestingPos(Backs *backs) {
  float x = 20;
  size_t y = CLUBS_Y + (CARD_HEIGHT*CARD_SCALE) + 10;
  for (int b = BC_RED; b < BC_COUNT; ++b) {
    Back *back = &hmget(backs, b);
    back->position = CLITERAL(Vector2) { x, y };
    x += (CARD_WIDTH*CARD_SCALE)+10; 
  }
}

void CreateBacks(Backs **backs, int y) {
  size_t x = 0;
  for (int b = BC_RED; b < BC_COUNT; ++b) {
    Rectangle r = { .x = x, .y = y, .width = CARD_WIDTH, .height = CARD_HEIGHT };
    Back back = { .source = r, .position = Vector2Zero() };
    hmput(*backs, b, back);
    x += (CARD_WIDTH + CARD_SPACING);
  }
}

bool DrawDeckItemToScreen(Texture2D tex, Vector2 pos, Rectangle src, Vector2 mouse) {
  Rectangle bounds = { .x = pos.x, .y = pos.y, .width = CARD_WIDTH*CARD_SCALE, .height = CARD_HEIGHT*CARD_SCALE };
  DrawTexturePro(tex, src, bounds, Vector2Zero(), 0, WHITE);
  return CheckCollisionPointRec(mouse, bounds);
}

void DrawHoveredOutline(Card *card) {
  Rectangle bounds = { .x = card->position.x, .y = card->position.y, .width = CARD_WIDTH*CARD_SCALE, .height = CARD_HEIGHT*CARD_SCALE };
  DrawRectangleLinesEx(bounds, 3, LIME);
}

int main(void) {
  InitWindow(1200, 720, "Cards");

  Image cardsImg = LoadImage("./assets/cards.png");
  Texture cardsTexture = LoadTextureFromImage(cardsImg);
  UnloadImage(cardsImg);
  
  Deck deck = {0};
  CreateDeck(&deck);
  InitPosForTesting(&deck);

  Image backsImg = LoadImage("./assets/backs.png");
  Texture backsTexture = LoadTextureFromImage(backsImg);
  UnloadImage(backsImg);
  Backs *backs = {0};
  CreateBacks(&backs, BACK_STYLE_H_MEANDER_Y);
  nob_log(NOB_INFO, "backs count: %ld", hmlen(backs));
  InitBacksTestingPos(backs);

  GameState gs = {0};
  gs.deck = deck;
  gs.backs = backs;
  gs.activeCard = NULL;
  gs.hoveredCard = NULL;

  while(!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(DARKGRAY);

    Vector2 mouse = GetMousePosition();
    Vector2 delta = GetMouseDelta();
    
    if (gs.activeCard) {
      if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        gs.activeCard = NULL;
      } else if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
          gs.activeCard->position = Vector2Add(gs.activeCard->position, delta);
      }
    }

    gs.hoveredCard = NULL;
    for (size_t c = 0; c < gs.deck.count; ++c) {

      Card *card = &gs.deck.items[c];
      if (gs.activeCard && card == gs.activeCard) {
        gs.hoveredCard = gs.activeCard;
        continue;
      }
      
      if (DrawDeckItemToScreen(cardsTexture, card->position, card->source, mouse) && !gs.activeCard)
        gs.hoveredCard = card;
    }

    if (gs.activeCard) {
      DrawDeckItemToScreen(cardsTexture, gs.activeCard->position, gs.activeCard->source, mouse);
    }
    if (gs.hoveredCard) {
      DrawHoveredOutline(gs.hoveredCard);
      if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
          gs.activeCard = gs.hoveredCard;
    }

    for (int b = BC_RED; b < BC_COUNT; ++b) {
      Back *back = &hmget(gs.backs, b);
      DrawDeckItemToScreen(backsTexture, back->position, back->source, mouse);
    }

    EndDrawing();
  }

  UnloadTexture(cardsTexture);
  UnloadTexture(backsTexture);

  CloseWindow();
}
