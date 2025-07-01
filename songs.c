#include "spotify.h"

Song** songs = NULL;
int num_songs = 0;
int songs_capacity = 0;

Playlist* playlists = NULL;
int num_playlists = 0;
int playlists_capacity = 0;

void init_library() {
	songs_capacity = INITIAL_CAPACITY;
	songs = (Song**)malloc(songs_capacity * sizeof(Song*));

	playlists_capacity = INITIAL_CAPACITY;
	playlists = (Playlist*)malloc(playlists_capacity * sizeof(Playlist));
}

void cleanup_library() {
	for (int i = 0; i < num_songs; i++) {
		free(songs[i]);
	}
	free(songs);

	for (int i = 0; i < num_playlists; i++) {
		free(playlists[i].songs);
	}
	free(playlists);
}

int generate_song_id() {
	return num_songs + 1;
}

void add_song() {
	if (num_songs == songs_capacity) {
		songs_capacity *= 2;
		songs = (Song**)realloc(songs, songs_capacity * sizeof(Song*));
	}

	Song* new_song = (Song*)malloc(sizeof(Song));

	printf("\n--- DODAVANJE PJESME ---\n");
	new_song->id = generate_song_id();

	printf("Naziv pjesme: ");
	scanf(" %[^\n]", new_song->title);

	printf("Izvodjac: ");
	scanf(" %[^\n]", new_song->artist);

	printf("Album: ");
	scanf(" %[^\n]", new_song->album);

	printf("Trajanje (u sekundama): ");
	scanf("%d", &new_song->duration);

	printf("Godina izdanja: ");
	scanf("%d", &new_song->year);

	songs[num_songs++] = new_song;
	printf("Pjesma uspjesno dodana! (ID: %d)\n", new_song->id);

	append_song_to_file(new_song);

}

void delete_song(int id) {
	int found_index = -1;
	for (int i = 0; i < num_songs; i++) {
		if (songs[i]->id == id) {
			found_index = i;
			break;
		}
	}

	if (found_index == -1) {
		printf("Pjesma s ID %d nije pronadjena.\n", id);
		return;
	}

	free(songs[found_index]);

	for (int i = found_index; i < num_songs - 1; i++) {
		songs[i] = songs[i + 1];
	}
	num_songs--;

	reorganize_song_ids();

	FILE* file = fopen("songs.txt", "w");
	if (file == NULL) {
		printf("Greska pri otvaranju datoteke za pisanje!\n");
		return;
	}
	for (int i = 0; i < num_songs; i++) {
		fprintf(file, "%d|%s|%s|%s|%d|%d\n",
			songs[i]->id,
			songs[i]->title,
			songs[i]->artist,
			songs[i]->album,
			songs[i]->duration,
			songs[i]->year);
	}

	fclose(file);
	printf("Pjesma s ID %d uspjesno obrisana!\n", id);
}

void reorganize_song_ids(void) {
	for (int i = 0; i < num_songs; i++) {
		songs[i]->id = i + 1;
	}
}

void search_songs() {
	char query[MAX_LENGTH];
	printf("\n--- PRETRAGA PJESAMA ---\n");
	printf("Unesite naziv pjesme ili izvodjaca: ");
	scanf(" %[^\n]", query);

	printf("\nRezultati pretrage:\n");
	bool found = false;

	char query_lower[MAX_LENGTH];
	strcpy(query_lower, query);
	to_lower_case(query_lower);

	for (int i = 0; i < num_songs; i++) {
		char title_lower[MAX_LENGTH];
		strcpy(title_lower, songs[i]->title);
		to_lower_case(title_lower);
		bool title_match = string_contains(title_lower, query_lower);

		bool artist_match = false;
		char artists_lower[MAX_LENGTH];
		strcpy(artists_lower, songs[i]->artist);
		to_lower_case(artists_lower);

		char* artist_token = strtok(artists_lower, ",");
		while (artist_token != NULL) {
			while (*artist_token == ' ') artist_token++;
			char* end = artist_token + strlen(artist_token) - 1;
			while (end > artist_token && *end == ' ') end--;
			*(end + 1) = '\0';

			if (strcmp(artist_token, query_lower) == 0) {
				artist_match = true;
				break;
			}
			artist_token = strtok(NULL, ",");
		}

		if (title_match || artist_match) {
			printf("%d. %s - %s (%d:%02d)\n",
				songs[i]->id,
				songs[i]->artist,
				songs[i]->title,
				songs[i]->duration / 60,
				songs[i]->duration % 60);
			found = true;
		}
	}

	if (!found) {
		printf("Nema rezultata za \"%s\"\n", query);
	}
}

void list_all_songs() {
	printf("\n--- SVE PJESME ---\n");
	for (int i = 0; i < num_songs; i++) {
		printf("%d. %s - %s [%s] (%d:%02d, %d)\n",
			songs[i]->id,
			songs[i]->title,
			songs[i]->artist,
			songs[i]->album,
			songs[i]->duration / 60,
			songs[i]->duration % 60,
			songs[i]->year);
	}
}

void create_playlist() {
	if (num_playlists == playlists_capacity) {
		playlists_capacity *= 2;
		playlists = (Playlist*)realloc(playlists, playlists_capacity * sizeof(Playlist));
	}

	printf("\n--- KREIRANJE PLAYLISTE ---\n");
	printf("Naziv playliste: ");
	scanf(" %[^\n]", playlists[num_playlists].name);

	playlists[num_playlists].num_songs = 0;
	playlists[num_playlists].capacity = INITIAL_CAPACITY;
	playlists[num_playlists].songs = (Song**)malloc(INITIAL_CAPACITY * sizeof(Song*));

	num_playlists++;
	printf("Playlista kreirana!\n");
}

void add_to_playlist() {
	if (num_playlists == 0) {
		printf("Prvo kreirajte playlistu!\n");
		return;
	}
	printf("\n--- DODAVANJE U PLAYLISTU ---\n");
	printf("Dostupne playliste:\n");
	for (int i = 0; i < num_playlists; i++) {
		printf("%d. %s\n", i + 1, playlists[i].name);
	}

	int playlist_choice;
	printf("Odabir playliste (1-%d): ", num_playlists);
	scanf("%d", &playlist_choice);
	playlist_choice--;

	if (playlist_choice < 0 || playlist_choice >= num_playlists) {
		printf("Neispravan odabir!\n");
		return;
	}

	list_all_songs();
	int song_id;
	printf("Unesite ID pjesme za dodavanje: ");
	scanf("%d", &song_id);

	for (int i = 0; i < num_songs; i++) {
		if (songs[i]->id == song_id) {
			Playlist* pl = &playlists[playlist_choice];

			if (pl->num_songs == pl->capacity) {
				pl->capacity *= 2;
				pl->songs = (Song**)realloc(pl->songs, pl->capacity * sizeof(Song*));
			}
			pl->songs[pl->num_songs++] = songs[i];
			printf("Pjesma dodana u playlistu!\n");
			return;
		}
	}

	printf("Pjesma s ID %d nije pronadjena.\n", song_id);

}
void to_lower_case(char* str) {
	for (int i = 0; str[i]; i++) {
		str[i] = tolower(str[i]);
	}
}

bool string_contains(const char* str, const char* substr) {
	char str_lower[MAX_LENGTH];
	char substr_lower[MAX_LENGTH];
	char temp_str[MAX_LENGTH * 2];
	char word[MAX_LENGTH];

	strcpy(str_lower, str);
	strcpy(substr_lower, substr);
	to_lower_case(str_lower);
	to_lower_case(substr_lower);

	sprintf(temp_str, " %s ", str_lower);

	sprintf(word, " %s ", substr_lower);

	return strstr(temp_str, word) != NULL;
}

void append_song_to_file(Song* s) {
	FILE* file = fopen("songs.txt", "a");
	if (file == NULL) {
		printf("Greška pri otvaranju datoteke za pisanje.\n");
		return;
	}

	fprintf(file, "%d|%s|%s|%s|%d|%d\n",
		s->id, s->title, s->artist, s->album, s->duration, s->year);

	fclose(file);
}

void load_songs_from_file() {
	FILE* file = fopen("songs.txt", "r");
	if (file == NULL) {
		return;
	}

	char line[512];
	while (fgets(line, sizeof(line), file)) {
		Song* s = (Song*)malloc(sizeof(Song));
		sscanf(line, "%d|%49[^|]|%49[^|]|%49[^|]|%d|%d",
			&s->id, s->title, s->artist, s->album, &s->duration, &s->year);

		if (num_songs == songs_capacity) {
			songs_capacity *= 2;
			songs = (Song**)realloc(songs, songs_capacity * sizeof(Song*));
		}

		songs[num_songs++] = s;
	}

	fclose(file);
}

void edit_song(int id) {
	Song* song = NULL;
	int index = -1;
	for (int i = 0; i < num_songs; i++) {
		if (songs[i]->id == id) {
			song = songs[i];
			index = i;
			break;
		}
	}

	if (song == NULL) {
		printf("Pjesma s ID %d nije pronadjena.\n", id);
		return;
	}

	printf("\n--- UREDIVANJE PJESME ---\n");
	printf("Trenutni podaci:\n");
	printf("1. Naziv: %s\n", song->title);
	printf("2. Izvodjac: %s\n", song->artist);
	printf("3. Album: %s\n", song->album);
	printf("4. Trajanje: %d sekundi (%d:%02d)\n", song->duration, song->duration / 60, song->duration % 60);
	printf("5. Godina: %d\n", song->year);
	printf("0. Odustani\n");

	int choice;
	printf("\nSto zelite urediti? (0-5): ");
	scanf("%d", &choice);

	switch (choice) {
	case 1:
		printf("Novi naziv: ");
		scanf(" %[^\n]", song->title);
		break;
	case 2:
		printf("Novi izvodjac: ");
		scanf(" %[^\n]", song->artist);
		break;
	case 3:
		printf("Novi album: ");
		scanf(" %[^\n]", song->album);
		break;
	case 4:
		printf("Novo trajanje (u sekundama): ");
		scanf("%d", &song->duration);
		break;
	case 5:
		printf("Nova godina: ");
		scanf("%d", &song->year);
		break;
	case 0:
		printf("Odustajem od uredivanja.\n");
		return;
	default:
		printf("Neispravan odabir!\n");
		return;
	}
	FILE* file = fopen("songs.txt", "w");
	if (file == NULL) {
		printf("Greska pri otvaranju datoteke za pisanje.\n");
		return;
	}

	for (int i = 0; i < num_songs; i++) {
		fprintf(file, "%d|%s|%s|%s|%d|%d\n",
			songs[i]->id,
			songs[i]->title,
			songs[i]->artist,
			songs[i]->album,
			songs[i]->duration,
			songs[i]->year);
	}

	fclose(file);
	printf("\nPjesma uspjesno uredena!\n");
	printf("AZurirana pjesma:\n");
	printf("%d. %s - %s [%s] (%d:%02d, %d)\n",
		song->id,
		song->title,
		song->artist,
		song->album,
		song->duration / 60,
		song->duration % 60,
		song->year);
}

void list_artist_songs(const char* artist_name) {
	printf("\nPjesme izvodjaca \"%s\":\n", artist_name);
	bool found = false;

	char search_artist[MAX_LENGTH];
	strcpy(search_artist, artist_name);
	to_lower_case(search_artist);

	for (int i = 0; i < num_songs; i++) {
		char current_artist[MAX_LENGTH];
		strcpy(current_artist, songs[i]->artist);
		to_lower_case(current_artist);

		char* token = strtok(current_artist, ",");
		while (token != NULL) {
			while (*token == ' ') token++;
			char* end = token + strlen(token) - 1;
			while (end > token && *end == ' ') end--;
			*(end + 1) = '\0';

			if (strcmp(token, search_artist) == 0) {
				printf("- %s (%d:%02d) [%s, %d]\n",
					songs[i]->title,
					songs[i]->duration / 60,
					songs[i]->duration % 60,
					songs[i]->album,
					songs[i]->year);
				found = true;
				break;
			}
			token = strtok(NULL, ",");
		}
	}

	if (!found) {
		printf("Nema pjesama za izvodjaca \"%s\"\n", artist_name);
	}
}