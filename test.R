
library(nflfastR)
library(dplyr)
library(readr)
library(tidyr)


# Load play-by-play data for multiple seasons
pbp <- load_pbp(c(2018, 2019, 2020, 2021, 2022, 2023, 2024))

# Filter for safety kicks (kickoffs after safeties)
safety_kicks <- pbp %>%
  filter(play_type == "kickoff" & lag(safety) == 1) %>%  # Kickoff immediately after a safety
  select(posteam, yardline_100, return_yards, touchback)

# Compute the starting yardline (adjusted for own field)
safety_kicks <- safety_kicks %>%
  mutate(
    start_yardline = case_when(
      touchback == 1 ~ 25,  # Touchbacks go to the 25
      is.na(return_yards) ~ 100 - yardline_100,  # No return, convert from yardline_100
      TRUE ~ 100 - (yardline_100 - return_yards)  # Convert to correct side of the field
    )
  )

# Compute the average starting field position
average_start_yardline <- mean(safety_kicks$start_yardline, na.rm = TRUE)

# Print result
print(paste("Average starting field position after safety kicks:", round(average_start_yardline, 2)))





'''# Load play-by-play data for multiple seasons
pbp_data <- load_pbp(c(2018, 2019, 2021, 2022, 2023, 2024))

# Filter and select relevant columns
pbp_subset <- pbp_data %>%
  filter(ydstogo >= 1 & ydstogo <= 20) %>%  # Keep only ydstogo between 1 and 20
  select(Down = down, Distance = ydstogo, Yardline = yardline_100, EPA = ep) %>%
  drop_na(Down, Distance, Yardline, EPA)  # Remove rows with missing values

# Compute average EPA for each Down, Distance, Yardline combination
epa_avg <- pbp_subset %>%
  group_by(Down, Distance, Yardline) %>%
  summarize(EPA = mean(EPA, na.rm = TRUE), .groups = "drop")  # Get mean EPA

# Save the aggregated data to a CSV file
write_csv(epa_avg, "nfl_pbp_data.csv")

# Print preview
print(head(epa_avg))'''