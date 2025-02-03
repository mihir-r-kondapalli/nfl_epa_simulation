# Load necessary libraries
library(nflfastR)
library(tidyverse)

# Load play-by-play data (latest season)
pbp <- load_pbp(c(2018, 2019, 2021, 2022, 2023))  

# Filter for relevant play types
pbp_filtered <- pbp %>%
  filter(play_type %in% c("pass", "run", "field_goal", "punt"),  # Include punts & FGs
         !is.na(epa),                      # Exclude missing EPA values
         down %in% 1:4,                    # Keep 1st to 4th downs
         ydstogo <= 20,                    # Keep distances up to 20 yards
         yardline_100 >= 1 & yardline_100 <= 99)  # Keep valid field positions

# Define a mapping of play types to numbers
play_type_map <- c("run" = 0, "pass" = 1, "field_goal" = 2, "punt" = 3)

# Compute the average EPA per (down, ydstogo, yardline_100, play_type)
epa_summary <- pbp_filtered %>%
  group_by(down, ydstogo, yardline_100, play_type) %>%  
  summarize(avg_epa = mean(epa, na.rm = TRUE), .groups = "drop")  # Compute avg EPA per group

# Select the best play type for each (down, ydstogo, yardline_100)
optimal_plays <- epa_summary %>%
  group_by(down, ydstogo, yardline_100) %>%  
  filter(avg_epa == max(avg_epa)) %>%  # Keep all play types with highest EPA
  ungroup() %>%
  mutate(play_type_num = play_type_map[as.character(play_type)]) %>%  # Assign numeric value
  select(down, ydstogo, yardline_100, play_type, play_type_num, avg_epa)  # Keep relevant columns

# View the results
print(optimal_plays)

# Save to CSV (optional)
write_csv(optimal_plays, "optimal_play_types.csv")
