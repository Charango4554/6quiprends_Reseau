BEGIN {
    FS = ";";

    if (nb_series == "")
        nb_series = 4;
    if (max_serie == "")
        max_serie = 5;
    print "joueur;parties;dernier;meilleur;pire;score_moyen;cartes_posees;cartes_ramassees;KD;victoires;plus_gros_ramassage;pile_favorite;nb_fois_6e";
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
    joueur  = $3;
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

    players[joueur] = 1;
    cartes_posees[joueur]++;
    pile_freq[joueur, ligne]++;
    dernier_score[joueur] = score;
    parties_jouees[joueur, partie] = 1;

    if (ramasse != 0) {
        cartes_ramassees[joueur] += current_size;
        if (current_size == max_serie)
            nb_sixieme[joueur]++;
        if (tetes > plus_gros[joueur])
            plus_gros[joueur] = tetes;
    }

    if (tour >= player_last_tour[joueur, partie]) {
        player_last_tour[joueur, partie] = tour;
        player_final_score[joueur, partie] = score;
    }

    if (tour >= final_tour[partie, joueur]) {
        final_tour[partie, joueur] = tour;
        final_score[partie, joueur] = score;
    }
}

END {
    for (key in final_score) {
        split(key, parts, SUBSEP);
        p = parts[1];
        s = final_score[key];
        if (!(p in best_party_score) || s < best_party_score[p])
            best_party_score[p] = s;
    }

    for (key in final_score) {
        split(key, parts, SUBSEP);
        p = parts[1];
        j = parts[2];
        if (final_score[key] == best_party_score[p])
            victoires[j]++;
    }

    n = 0;
    for (p in players) {
        n++;
        order[n] = p;
    }

    if (n == 0)
        exit 0;

    for (i = 1; i <= n; i++)
        for (j = i + 1; j <= n; j++)
            if (order[j] < order[i]) {
                tmp = order[i];
                order[i] = order[j];
                order[j] = tmp;
            }

    for (pos = 1; pos <= n; pos++) {
        player = order[pos];
        parties = 0;
        total_score = 0;
        best = "";
        worst = "";

        for (key in parties_jouees) {
            split(key, parts, SUBSEP);
            j = parts[1];
            partie = parts[2];
            if (j != player)
                continue;
            parties++;
            score_val = player_final_score[j, partie];
            total_score += score_val;
            if (best == "" || score_val < best)
                best = score_val;
            if (worst == "" || score_val > worst)
                worst = score_val;
        }

        if (parties == 0) {
            best = 0;
            worst = 0;
        }

        moyenne = (parties ? total_score / parties : 0);
        if (cartes_ramassees[player] > 0) {
            kd = cartes_posees[player] / cartes_ramassees[player];
        } else {
            kd = cartes_posees[player];
        }

        pile_label = "ligne_-";
        pile_freq_best = -1;
        pile_idx = -1;
        for (key in pile_freq) {
            split(key, parts, SUBSEP);
            j = parts[1];
            ligne = parts[2];
            if (j != player)
                continue;
            freq = pile_freq[key];
            ligne_num = ligne + 0;
            if (freq > pile_freq_best || (freq == pile_freq_best && ligne_num < pile_idx)) {
                pile_freq_best = freq;
                pile_idx = ligne_num;
                pile_label = sprintf("ligne_%d", ligne_num);
            }
        }

        if (cartes_posees[player] == 0)
            pile_label = "ligne_-";

        printf("%s;%d;%d;%d;%d;%.2f;%d;%d;%.2f;%d;%d;%s;%d\n",
               player,
               parties,
               dernier_score[player] + 0,
               best + 0,
               worst + 0,
               moyenne,
               cartes_posees[player] + 0,
               cartes_ramassees[player] + 0,
               kd,
               victoires[player] + 0,
               plus_gros[player] + 0,
               pile_label,
               nb_sixieme[player] + 0);
    }
}
