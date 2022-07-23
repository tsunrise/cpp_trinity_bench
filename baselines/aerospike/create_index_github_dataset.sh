manage sindex create numeric stars_index ns github set github_macro bin stars
manage sindex create numeric forks_index ns github set github_macro bin forks
manage sindex create numeric adds_index ns github set github_macro bin adds
manage sindex create numeric dels_index ns github set github_macro bin dels
manage sindex create numeric add_del_ratio_index ns github set github_macro bin add_del_ratio
manage sindex create numeric events_count_index ns github set github_macro bin events_count
manage sindex create numeric issues_index ns github set github_macro bin issues
manage sindex create numeric start_date_index ns github set github_macro bin start_date
manage sindex create numeric end_date_index ns github set github_macro bin end_date

manage sindex delete adds_index ns tpch
manage sindex delete dels_index ns tpch
manage sindex delete add_del_ratio_index ns tpch
