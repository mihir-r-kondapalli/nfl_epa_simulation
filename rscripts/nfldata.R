# Load required libraries
options(repos = c(CRAN = "https://cran.rstudio.com/"))
library(nflfastR)
library(tidyverse)

# Load 2024 play-by-play data
pbp <- load_pbp(2024)

# Filter and summarize EP by down-distance-yardline combo
ep_summary <- pbp %>%
  filter(
    season == 2024,
    season_type == "REG",
    !is.na(down), down >= 1 & down <= 4,
    !is.na(ydstogo), ydstogo >= 1 & ydstogo <= 20,
    !is.na(yardline_100), yardline_100 >= 1 & yardline_100 <= 99,
    !is.na(ep)
  ) %>%
  mutate(
    Down = as.integer(down),
    Distance = as.integer(ydstogo),
    Yardline = as.integer(yardline_100)
  ) %>%
  group_by(Down, Distance, Yardline) %>%
  summarize(EP = mean(ep), .groups = "drop") %>%
  arrange(Down, Distance, Yardline)

# Save to CSV
write_csv(ep_summary, "nfl_pbp_data.csv")
cat("✅ Saved EP summary to nfl_pbp_data.csv with", nrow(ep_summary), "rows.\n")
