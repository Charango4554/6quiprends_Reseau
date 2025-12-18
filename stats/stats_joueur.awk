BEGIN {
    FS = ";";

    if (joueur == "") {
        print "stats_joueur.awk : utiliser -v joueur=<id>" > "/dev/stderr";
        abort_script = 1;
        exit 1;
    }

    if (login == "")
        login = sprintf("player_%s", joueur);

    if (out_dir == "") {
        if (system("test -d ../stats/USER") == 0)
            out_dir = "../stats/USER";
        else
            out_dir = "../stats/USERS";
    }

    if (nb_series == "")
        nb_series = 4;

    if (max_serie == "")
        max_serie = 5;

    cartes_posees = 0;
    cartes_ramassees = 0;
    nb_sixieme = 0;
    plus_gros_ramassage = 0;
    dernier_score = 0;
}

function init_party(p,    i) {
    if (!(p in party_initialized)) {
        for (i = 0; i < nb_series; i++)
            line_size[p, i] = 1;
        party_initialized[p] = 1;
    }
}

NF != 8 { next }

{
    partie  = $1 + 0;
    tour    = $2 + 0;
    joueur_id = $3;
    carte   = $4 + 0;
    ligne   = $5 + 0;
    ramasse = $6 + 0;
    tetes   = $7 + 0;
    score   = $8 + 0;

    init_party(partie);

    key = partie SUBSEP ligne;
    current_size = line_size[key];
    if (current_size == 0)
        current_size = 1;

    if (ramasse == 0) {
        current_size++;
        if (current_size > max_serie)
            current_size = max_serie;
        line_size[key] = current_size;
    } else {
        line_size[key] = 1;
    }

    if (joueur_id == joueur) {
        cartes_posees++;
        pile_freq[ligne]++;
        dernier_score = score;
        parties_jouees[partie] = 1;

        if (ramasse != 0) {
            cartes_ramassees += current_size;
            if (current_size == max_serie)
                nb_sixieme++;
            if (tetes > plus_gros_ramassage)
                plus_gros_ramassage = tetes;
        }

        if (tour >= player_last_tour[partie]) {
            player_last_tour[partie] = tour;
            player_final_score[partie] = score;
        }
    }

    if (tour >= final_tour[partie, joueur_id]) {
        final_tour[partie, joueur_id] = tour;
        final_score[partie, joueur_id] = score;
    }
}

END {
    if (abort_script)
        exit 1;

    parties = 0;
    score_total = 0;
    best_score = "";
    worst_score = "";

    for (p in parties_jouees) {
        parties++;
        score_val = player_final_score[p];
        score_total += score_val;

        if (best_score == "" || score_val < best_score)
            best_score = score_val;

        if (worst_score == "" || score_val > worst_score)
            worst_score = score_val;
    }

    if (parties == 0) {
        best_score = 0;
        worst_score = 0;
    }

    score_moyen = (parties ? score_total / parties : 0);

    for (key in final_score) {
        split(key, idx, SUBSEP);
        p = idx[1];
        s = final_score[key];
        if (!(p in best_party_score) || s < best_party_score[p])
            best_party_score[p] = s;
    }

    victoires = 0;
    for (p in parties_jouees)
        if (p in best_party_score && player_final_score[p] == best_party_score[p])
            victoires++;

    pile_label = "ligne_-";
    pile_best = -1;
    pile_freq_best = -1;
    for (line in pile_freq) {
        freq = pile_freq[line];
        line_num = line + 0;
        if (freq > pile_freq_best || (freq == pile_freq_best && line_num < pile_best)) {
            pile_freq_best = freq;
            pile_best = line_num;
            pile_label = sprintf("ligne_%d", line_num);
        }
    }

    if (cartes_posees == 0)
        pile_label = "ligne_-";

    kd = (cartes_ramassees > 0) ? cartes_posees / cartes_ramassees : cartes_posees;

    user_file = out_dir "/" login ".txt";

    pwd_line = "";
    while ((getline line < user_file) > 0) {
        if (line ~ /^PASSWORD=/) {
            pwd_line = line;
            break;
        }
    }
    close(user_file);

    if (pwd_line == "")
        pwd_line = "PASSWORD=";

    printf("%s\n", pwd_line) > user_file;
    printf("PARTIES=%d\n", parties) >> user_file;
    printf("DERNIER_SCORE=%d\n", dernier_score) >> user_file;
    printf("MEILLEUR_SCORE=%d\n", best_score) >> user_file;
    printf("Pire_SCORE=%d\n", worst_score) >> user_file;
    printf("SCORE_MOYEN=%.2f\n", score_moyen) >> user_file;
    printf("CARTES_POSEES=%d\n", cartes_posees) >> user_file;
    printf("CARTES_RAMASSEES=%d\n", cartes_ramassees) >> user_file;
    printf("KD=%.2f\n", kd) >> user_file;
    printf("VICTOIRES=%d\n", victoires) >> user_file;
    printf("PLUS_GROS_RAMASSAGE=%d\n", plus_gros_ramassage) >> user_file;
    printf("PILE_FAVORITE=%s\n", pile_label) >> user_file;
    printf("NB_FOIS_6E_CARTE=%d\n", nb_sixieme) >> user_file;
    close(user_file);
}
