#define _CRT_SECURE_NO_WARNINGS

#ifndef SPOTIFY_H
#define SPOTIFY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#define MAX_LENGTH 100
#define INITIAL_CAPACITY 10

typedef struct {
	char title[MAX_LENGTH];
	char artist[MAX_LENGTH];
	char album[MAX_LENGTH];
	int duration; // u sekundama
	int year;
	int id;
} Song;

typedef struct {
	char name[MAX_LENGTH];
	Song** songs;
	int num_songs;
	int capacity;
} Artist;

typedef struct {
	char name[MAX_LENGTH];
	Song** songs;
	int num_songs;
	int capacity;
} Playlist;

// Osnovne funkcije
void init_library();
void cleanup_library();
int generate_song_id();

// Funkcije za pjesme
void add_song();
void delete_song(int id);
void search_songs();
void list_all_songs();
void edit_song(int id);
void reorganize_song_ids(void);

// Funkcije za izvo?a?e
void list_artists();
void list_artist_songs(const char* artist_name);

// Funkcije za playliste
void create_playlist();
void add_to_playlist();
void list_playlists();
void show_playlist_songs(const char* playlist_name);

void append_song_to_file(Song* s);
void load_songs_from_file();

// Pomocne funkcije
void to_lower_case(char* str);
bool string_contains(const char* str, const char* substr);

extern Song** songs;
extern int num_songs;
extern int songs_capacity;

extern Playlist* playlists;
extern int num_playlists;
extern int playlists_capacity;

#endif // SPOTIFY_H