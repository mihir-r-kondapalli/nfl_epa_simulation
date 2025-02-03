# Load required libraries
library(nflfastR)
library(dplyr)
library(jsonlite)

# Load NFL play-by-play data for the latest season (change year if needed)
pbp_data <- load_pbp(c(2018, 2019, 2021, 2022, 2023, 2024))

# Filter for field goal attempts and results
fg_data <- pbp_data %>%
  filter(play_type == "field_goal") %>%
  select(yardline_100, field_goal_result)

# Convert field goal result to binary (1 = made, 0 = missed)
fg_data <- fg_data %>%
  mutate(fg_made = ifelse(field_goal_result == "made", 1, 0))

# Compute field goal probability and number of attempts for each yard line
fg_prob <- fg_data %>%
  group_by(yardline_100) %>%
  summarise(
    attempts = n(),         # Count number of FG attempts
    makes = sum(fg_made),   # Count number of successful kicks
    fg_prob = makes / attempts # Compute field goal probability
  ) %>%
  arrange(yardline_100)

# Convert to a list of key-value pairs including attempts
fg_prob_list <- lapply(1:nrow(fg_prob), function(i) {
  list(
    yardline = fg_prob$yardline_100[i],
    attempts = fg_prob$attempts[i],
    fg_prob = fg_prob$fg_prob[i]
  )
})

# Convert to JSON format with newlines and proper formatting
fg_prob_json <- toJSON(fg_prob_list, pretty = TRUE, auto_unbox = TRUE)

# Save to a JSON file
write(fg_prob_json, file = "field_goal_probabilities.json")

# Print confirmation message
cat("Field goal probabilities saved to 'field_goal_probabilities.json'\n")
