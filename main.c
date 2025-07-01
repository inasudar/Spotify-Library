#include "spotify.h"

void display_menu() {
	printf("\n=== SPOTIFY BIBLIOTEKA ===\n");
	printf("1. Dodaj pjesmu\n");
	printf("2. Pregledaj sve pjesme\n");
	printf("3. Pretrazi pjesme\n");
	printf("4. Obrisi pjesmu\n");
	printf("5. Uredi pjesmu\n");
	printf("6. Kreiraj playlistu\n");
	printf("7. Dodaj pjesmu u playlistu\n");
	printf("8. Pregledaj playliste\n");
	printf("0. Izlaz\n");
	printf("Odabir: ");
}

int main() {
	init_library();
	load_songs_from_file();

	int choice;

	do {
		display_menu();
		scanf("%d", &choice);

		switch (choice) {
		case 1: add_song(); break;
		case 2: list_all_songs(); break;
		case 3: search_songs(); break;
		case 4: {
			printf("\n--- POPIS PJESAMA ---\n");
			for (int i = 0; i < num_songs; i++) {
				printf("%d %s - %s (%d:%02d)\n",
					songs[i]->id,
					songs[i]->artist,
					songs[i]->title,
					songs[i]->duration / 60,
					songs[i]->duration % 60);
			}

			int id;
			printf("\nUnesite ID pjesme za brisanje: ");
			scanf("%d", &id);
			delete_song(id);
			break;
		}
		case 5: {
			printf("\n--- POPIS PJESAMA ---\n");
			for (int i = 0; i < num_songs; i++) {
				printf("%d %s - %s (%d:%02d)\n",
					songs[i]->id,
					songs[i]->artist,
					songs[i]->title,
					songs[i]->duration / 60,
					songs[i]->duration % 60);
			}

			int id;
			printf("\nUnesite ID pjesme za uredivanje: ");
			scanf("%d", &id);
			edit_song(id);
			break;
		}
		case 6: create_playlist(); break;
		case 7: add_to_playlist(); break;
		case 8: {
			printf("\n--- PLAYLISTE ---\n");
			for (int i = 0; i < num_playlists; i++) {
				// Pravilno sklanjanje rije?i "pjesma" ovisno o broju
				char pjesma_str[20];
				if (playlists[i].num_songs == 1) {
					strcpy(pjesma_str, "1 pjesma");
				}
				else if (playlists[i].num_songs >= 2 && playlists[i].num_songs <= 4) {
					sprintf(pjesma_str, "%d pjesme", playlists[i].num_songs);
				}
				else {
					sprintf(pjesma_str, "%d pjesama", playlists[i].num_songs);
				}

				printf("\n%d. %s (%s):\n",
					i + 1,
					playlists[i].name,
					pjesma_str);

				// Ispis svih pjesama u playlisti
				for (int j = 0; j < playlists[i].num_songs; j++) {
					printf("   - %s - %s (%d:%02d)\n",
						playlists[i].songs[j]->title,
						playlists[i].songs[j]->artist,
						playlists[i].songs[j]->duration / 60,
						playlists[i].songs[j]->duration % 60);
				}
			}
			break;
		}
		case 0: printf("Izlazim...\n"); break;
		default: printf("Nepoznata opcija!\n");
		}
	} while (choice != 0);

	cleanup_library();
	return 0;
}