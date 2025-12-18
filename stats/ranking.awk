BEGIN {
    FS = ";";
    print "rang;joueur;score_moyen;parties;victoires";
}

NF != 8 { next }

{
    partie = $1 + 0;
    tour   = $2 + 0;
    joueur = $3;
    score  = $8 + 0;

    if (tour >= last_tour[partie, joueur]) {
        last_tour[partie, joueur] = tour;
        final_score[partie, joueur] = score;
        has_records = 1;
    }

    participants[joueur] = 1;
}

END {
    if (!has_records)
        exit 0;

    for (key in final_score) {
        split(key, parts, SUBSEP);
        partie = parts[1];
        score = final_score[key];
        if (!(partie in best_party_score) || score < best_party_score[partie])
            best_party_score[partie] = score;
    }

    for (key in final_score) {
        split(key, parts, SUBSEP);
        partie = parts[1];
        joueur = parts[2];
        parties[joueur]++;
        total_score[joueur] += final_score[key];
        if (final_score[key] == best_party_score[partie])
            victoires[joueur]++;
    }

    n = 0;
    for (p in participants) {
        n++;
        order[n] = p;
    }

    for (i = 1; i <= n; i++)
        for (j = i + 1; j <= n; j++) {
            player_a = order[i];
            player_b = order[j];
            avg_a = (parties[player_a] ? total_score[player_a] / parties[player_a] : 0);
            avg_b = (parties[player_b] ? total_score[player_b] / parties[player_b] : 0);
            if (avg_b < avg_a || (avg_a == avg_b && player_b < player_a)) {
                tmp = order[i];
                order[i] = order[j];
                order[j] = tmp;
            }
        }

    rank = 1;
    for (pos = 1; pos <= n; pos++) {
        player = order[pos];
        avg = (parties[player] ? total_score[player] / parties[player] : 0);
        printf("%d;%s;%.2f;%d;%d\n",
               rank++,
               player,
               avg,
               parties[player] + 0,
               victoires[player] + 0);
    }
}
