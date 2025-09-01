#include "spotify.h"

static void display_menu(void) {
    printf("\n=== SPOTIFY BIBLIOTEKA ===\n");
    printf("1. Dodaj pjesmu\n");
    printf("2. Pregledaj sve pjesme\n");
    printf("3. Pretrazi pjesme\n");
    printf("4. Obrisi pjesmu\n");
    printf("5. Uredi pjesmu\n");
    printf("6. Kreiraj playlistu\n");
    printf("7. Dodaj pjesmu u playlistu\n");
    printf("8. Pregledaj playliste\n");
    printf("9. Sortiraj\n");
    printf("10. Umetni pjesmu po naslovu\n");
    printf("0. Izlaz\n");
    printf("Odabir: ");
}

int main(void) {
    init_library();
    load_songs_from_file();

    int choice;
    do {
        display_menu();
        if (scanf("%d", &choice) != 1) {
            // ocisti neispravan unos
            int c; while ((c = getchar()) != '\n' && c != EOF) {}
            puts("Neispravan unos. Pokusaj ponovo.");
            continue;
        }

        switch ((Menu)choice) {
        case MENU_ADD:      add_song(); break;
        case MENU_LIST:     list_all_songs(); break;
        case MENU_SEARCH:   search_songs(); break;
        case MENU_DELETE: {
            printf("\n--- POPIS PJESAMA ---\n");
            for (int i = 0; i < num_songs; i++) {
                char tbuf[8]; format_duration_mmss(songs[i]->duration, tbuf); // unija u akciji
                printf("%d %s - %s (%s)\n",
                    songs[i]->id, songs[i]->artist, songs[i]->title, tbuf);
            }
            int id;
            printf("\nUnesite ID pjesme za brisanje: ");
            if (scanf("%d", &id) == 1) delete_song(id);
            else puts("Neispravan ID.");
            break;
        }
        case MENU_EDIT: {
            printf("\n--- POPIS PJESAMA ---\n");
            for (int i = 0; i < num_songs; i++) {
                char tbuf[8]; format_duration_mmss(songs[i]->duration, tbuf); // unija u akciji
                printf("%d %s - %s (%s)\n",
                    songs[i]->id, songs[i]->artist, songs[i]->title, tbuf);
            }
            int id;
            printf("\nUnesite ID pjesme za uredivanje: ");
            if (scanf("%d", &id) == 1) edit_song(id);
            else puts("Neispravan ID.");
            break;
        }
        case MENU_CREATE_PL:  create_playlist(); break;
        case MENU_ADD_TO_PL:  add_to_playlist(); break;
        case MENU_VIEW_PL: list_playlists(); break;
        case MENU_SORT:    sort_songs_menu(); break;
        case MENU_INSERT:  add_song_insert_sorted_by_title(); break;

        case MENU_EXIT:
            printf("Izlazim...\n"); break;

        default:
            printf("Nepoznata opcija!\n");
        }

    } while (choice != 0);

    cleanup_library();
    return 0;
}
