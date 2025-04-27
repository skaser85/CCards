#include "raylib.h"
#include "raymath.h"

#define STB_DS_IMPLEMENTATION
#include "../third-party/stb/stb_ds.h"

#define NOB_IMPLEMENTATION
#include "../nob.h"

#define SCREEN_WIDTH 1600
#define SCREEN_HEIGHT 960

#define SRC_CARD_WIDTH 200
#define SRC_CARD_HEIGHT 350
#define SRC_CARD_SPACING 8
#define SRC_CARD_SCALE 0.5

#define CARD_WIDTH (SRC_CARD_WIDTH*SRC_CARD_SCALE)
#define CARD_HEIGHT (SRC_CARD_HEIGHT*SRC_CARD_SCALE)

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
  bool drawn;
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
  Deck drawn;
  Card *hoveredCard;
  Card *activeCard;
  Backs *backs;
  Back *activeBack;
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
      Rectangle source = { .x = x, .y = y, .width = SRC_CARD_WIDTH, .height = SRC_CARD_HEIGHT };
      Card c = { .suit = s, .value = v, .source = source, .position = Vector2Zero() };
      nob_da_append(deck, c);
      x += (SRC_CARD_WIDTH + SRC_CARD_SPACING);
    }
  }
}

void InitPosForTesting(Deck *deck) {
  float x = 20;
  float y = 10;
  for (size_t c = 0; c < deck->count; ++c) {
    if (c > 0 && c % 13 == 0) {
      y += (CARD_HEIGHT) + 10;
      x = 20;
    }
    deck->items[c].position = CLITERAL(Vector2) { x, y };
    x += (CARD_WIDTH)+10;
  }
}

void InitBacksTestingPos(Backs *backs) {
  float x = 20;
  size_t y = (CARD_HEIGHT) + 10;
  for (int b = BC_RED; b < BC_COUNT; ++b) {
    Back *back = &hmget(backs, b);
    back->position = CLITERAL(Vector2) { x, y };
    x += (CARD_WIDTH)+10; 
  }
}

void CreateBacks(Backs **backs, int y) {
  size_t x = 0;
  for (int b = BC_RED; b < BC_COUNT; ++b) {
    Rectangle r = { .x = x, .y = y, .width = SRC_CARD_WIDTH, .height = SRC_CARD_HEIGHT };
    Back back = { .source = r, .position = Vector2Zero() };
    hmput(*backs, b, back);
    x += (SRC_CARD_WIDTH + SRC_CARD_SPACING);
  }
}

bool DrawDeckItemToScreen(Texture2D tex, Vector2 pos, Rectangle src, Vector2 mouse) {
  Rectangle bounds = { .x = pos.x, .y = pos.y, .width = CARD_WIDTH, .height = CARD_HEIGHT };
  DrawTexturePro(tex, src, bounds, Vector2Zero(), 0, WHITE);
  return CheckCollisionPointRec(mouse, bounds);
}

void DrawHoveredOutline(Card *card) {
  Rectangle bounds = { .x = card->position.x, .y = card->position.y, .width = CARD_WIDTH, .height = CARD_HEIGHT };
  DrawRectangleLinesEx(bounds, 3, LIME);
}

Card *GetNextCard(Deck *src, Deck *dest) {
  nob_da_append(dest, src->items[0]);
  src->count--;
  if (src->count == 0) return &dest->items[dest->count-1];
  for (size_t c = 1; c < src->count-1; ++c) {
    src->items[c-1] = src->items[c];
  }
  return &dest->items[dest->count-1];
}

Card *GetRandomCard(Deck *deck) {
  Card *card = NULL;
  while (!card) {
    int v = GetRandomValue(0, deck->count);
    if (!deck->items[v].drawn) {
      card = &deck->items[v];
      card->drawn = true;
      break;
    }
  }
  return card;
}

void ShuffleDeck(Deck *deck) {
  Deck temp = {0};
  for (size_t c = 0; c < deck->count; ++c) {
    Card *card = GetRandomCard(deck);
    nob_da_append(&temp, *card);
  }
  deck->count = 0;
  for (size_t c = 0; c < temp.count; ++c) {
    Card card = temp.items[c];
    nob_da_append(deck, card);
  }
}

const char* sizetToString(size_t num, size_t len) {
  char* buffer = (char*)malloc((len+1)*sizeof(char));
  if (!buffer) return NULL;
  snprintf(buffer, (len+1), "%ld", num);
  return buffer;
}

int main(void) {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Cards");

  Image cardsImg = LoadImage("./assets/cards.png");
  Texture cardsTexture = LoadTextureFromImage(cardsImg);
  UnloadImage(cardsImg);
  
  Deck deck = {0};
  CreateDeck(&deck);
  ShuffleDeck(&deck);
  //InitPosForTesting(&deck);
  Deck drawn = {0};

  Image backsImg = LoadImage("./assets/backs.png");
  Texture backsTexture = LoadTextureFromImage(backsImg);
  UnloadImage(backsImg);
  Backs *backs = {0};
  CreateBacks(&backs, BACK_STYLE_H_MEANDER_Y);
  //InitBacksTestingPos(backs);

  GameState gs = {0};
  gs.deck = deck;
  gs.drawn = drawn;
  gs.backs = backs;
  gs.activeCard = NULL;
  gs.hoveredCard = NULL;
  gs.activeBack = &hmget(backs, BC_BLUE);

  while(!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(DARKGRAY);

    Vector2 mouse = GetMousePosition();
    Vector2 delta = GetMouseDelta();
 
    Rectangle deckBounds = { .x = 10, .y = 20, .width = (CARD_WIDTH)*1.25, .height = (CARD_HEIGHT)*1.15 };
    DrawRectangleLinesEx(deckBounds, 5, DARKPURPLE);
    
    if (gs.deck.count > 0) {
      Vector2 backPos = { .x = deckBounds.x + (deckBounds.width-(CARD_WIDTH))/2, .y = deckBounds.y + (deckBounds.height-(CARD_HEIGHT))/2 };
      gs.activeBack->position = backPos;
      DrawDeckItemToScreen(backsTexture, backPos, gs.activeBack->source, mouse);

      if (CheckCollisionPointRec(mouse, deckBounds) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Card *card = GetNextCard(&gs.deck, &gs.drawn); 
        card->position = CLITERAL(Vector2) { .x = deckBounds.x + deckBounds.width + 25, .y = backPos.y };
      }
    } else {
      if (CheckCollisionPointRec(mouse, deckBounds) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        for (size_t c = 0; c < gs.drawn.count; ++c) {
          Card card = gs.drawn.items[c];
          card.drawn = false;
          nob_da_append(&gs.deck, card);
        }
        gs.drawn.count = 0;
      }
    }

    //const char* text = sizetToString(gs.deck.count, 2);
    //DrawText(text, deckBounds.x, deckBounds.y+deckBounds.height+50, 30, LIME);

    for (size_t c = 0; c < gs.drawn.count; ++c) {
      Card card = gs.drawn.items[c];
      DrawDeckItemToScreen(cardsTexture, card.position, card.source, mouse);
    }

    EndDrawing();
  }

  UnloadTexture(cardsTexture);
  UnloadTexture(backsTexture);

  CloseWindow();
}


    //if (gs.activeCard) {
    //  if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
    //    gs.activeCard = NULL;
    //  } else if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
    //      gs.activeCard->position = Vector2Add(gs.activeCard->position, delta);
    //  }
    //}

    //gs.hoveredCard = NULL;
    //for (size_t c = 0; c < gs.deck.count; ++c) {

    //  Card *card = &gs.deck.items[c];
    //  if (gs.activeCard && card == gs.activeCard) {
    //    gs.hoveredCard = gs.activeCard;
    //    continue;
    //  }
    //  
    //  if (DrawDeckItemToScreen(cardsTexture, card->position, card->source, mouse) && !gs.activeCard)
    //    gs.hoveredCard = card;
    //}

    //if (gs.activeCard) {
    //  DrawDeckItemToScreen(cardsTexture, gs.activeCard->position, gs.activeCard->source, mouse);
    //}
    //if (gs.hoveredCard) {
    //  DrawHoveredOutline(gs.hoveredCard);
    //  if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
    //      gs.activeCard = gs.hoveredCard;
    //}
   //
   //for (int b = BC_RED; b < BC_COUNT; ++b) {
   //  Back *back = &hmget(gs.backs, b);
   //  DrawDeckItemToScreen(backsTexture, back->position, back->source, mouse);
   //}
