# Load necessary libraries
options(repos = c(CRAN = "https://cran.rstudio.com/"))
library(nflfastR)
library(tidyverse)
library(jsonlite)

# --- Command line argument: sample size threshold ---
args <- commandArgs(trailingOnly = TRUE)
threshold <- if (length(args) == 0) 20 else as.numeric(args[1])

# --- Interpolation helpers ---

# Interpolate missing vectors from immediate neighbors (distance ±1)
interpolate_missing_vectors <- function(distributions, all_keys, threshold) {
  for (k in all_keys$key) {
    if (is.null(distributions[[k]])) {
      parts <- strsplit(k, "-")[[1]]
      d <- as.integer(parts[1])
      y <- as.integer(parts[2])
      neighbor_keys <- paste(d, c(y - 1, y + 1), sep = "-")
      neighbor_samples <- unlist(lapply(neighbor_keys, function(nk) distributions[[nk]]), use.names = FALSE)

      if (length(neighbor_samples) > 0) {
        interpolated <- if (length(neighbor_samples) < threshold) {
          sample(neighbor_samples, size = threshold, replace = TRUE)
        } else {
          neighbor_samples
        }
        distributions[[k]] <- interpolated
      }
    }
  }
  return(distributions)
}

# Fill in any remaining missing keys using fallback pool or zeros
fill_missing_defaults <- function(distributions, all_keys, fallback_pool, threshold) {
  for (k in all_keys$key) {
    if (is.null(distributions[[k]])) {
      filled <- if (length(fallback_pool) > 0) {
        sample(fallback_pool, size = threshold, replace = TRUE)
      } else {
        rep(0, threshold)
      }
      distributions[[k]] <- filled
    }
  }
  return(distributions)
}

# Pad existing keys to ensure all vectors have at least `threshold` values
pad_short_keys <- function(distributions, all_keys, threshold) {
  for (k in all_keys$key) {
    if (!is.null(distributions[[k]])) {
      vec <- distributions[[k]]
      if (length(vec) < threshold) {
        distributions[[k]] <- sample(vec, size = threshold, replace = TRUE)
      }
    }
  }
  return(distributions)
}

# --- Load play-by-play data ---
pbp_data <- load_pbp(c(2024))

# --- Define yardline bins ---
individual_bins <- setNames(as.list(1:20), as.character(1:20))
grouped_bins <- list(
  "21-23"  = 21:23,
  "24-27"  = 24:27,
  "28-32"  = 28:32,
  "33-38"  = 33:38,
  "39-44"  = 39:44,
  "45-50"  = 45:50,
  "51-70"  = 51:70,
  "71-85"  = 71:85,
  "86-99"  = 86:99
)
yardline_bins <- c(individual_bins, grouped_bins)

# All down-distance combinations
all_keys <- expand.grid(down = 1:4, distance = 1:20) %>%
  mutate(key = paste(down, distance, sep = "-"))

# --- Loop over yardline bins ---
for (yardline_range in names(yardline_bins)) {
  lower_yl <- min(yardline_bins[[yardline_range]])
  upper_yl <- max(yardline_bins[[yardline_range]])

  rush_distributions <- list()
  pass_distributions <- list()

  for (input_down in 1:4) {
    for (yards in 1:20) {
      key <- paste(input_down, yards, sep = "-")

      # Extract RUSH plays
      rush_yards <- pbp_data %>%
        filter(
          play_type == "run",
          qb_scramble == 0,
          down == input_down,
          ydstogo == yards,
          yardline_100 >= lower_yl, yardline_100 <= upper_yl,
          game_seconds_remaining >= 300,
          season_type == "REG"
        ) %>%
        mutate(
          yards_gained = ifelse(fumble_lost == 1, -1100 - return_yards + yards_gained, yards_gained),
          yards_gained = ifelse(rush_touchdown == 1, 10 + yards_gained, yards_gained)
        ) %>%
        pull(yards_gained)

      # Extract PASS plays
      pass_yards <- pbp_data %>%
        filter(
          (play_type == "pass" | qb_scramble == 1),
          down == input_down,
          ydstogo == yards,
          yardline_100 >= lower_yl, yardline_100 <= upper_yl,
          game_seconds_remaining >= 300,
          season_type == "REG"
        ) %>%
        mutate(
          yards_gained = ifelse(fumble_lost == 1, -1100 - return_yards + yards_gained, yards_gained),
          yards_gained = ifelse(interception == 1, -2100 - return_yards + air_yards, yards_gained),
          yards_gained = ifelse(pass_touchdown == 1 | rush_touchdown == 1, 10 + yards_gained, yards_gained)
        ) %>%
        pull(yards_gained)

      if (length(rush_yards) > 0) rush_distributions[[key]] <- rush_yards
      if (length(pass_yards) > 0) pass_distributions[[key]] <- pass_yards

      print(paste(yardline_range, ": ", key))
    }
  }

  # --- Interpolate from neighbors ---
  rush_distributions <- interpolate_missing_vectors(rush_distributions, all_keys, threshold)
  pass_distributions <- interpolate_missing_vectors(pass_distributions, all_keys, threshold)

  # --- Build fallback pools for final gap-fill ---
  rush_pool <- pbp_data %>%
    filter(
      play_type == "run",
      qb_scramble == 0,
      yardline_100 >= lower_yl, yardline_100 <= upper_yl,
      game_seconds_remaining >= 300,
      season_type == "REG"
    ) %>%
    mutate(
      yards_gained = ifelse(fumble_lost == 1, -1100 - return_yards + yards_gained, yards_gained),
      yards_gained = ifelse(rush_touchdown == 1, 10 + yards_gained, yards_gained)
    ) %>%
    pull(yards_gained)

  pass_pool <- pbp_data %>%
    filter(
      (play_type == "pass" | qb_scramble == 1),
      yardline_100 >= lower_yl, yardline_100 <= upper_yl,
      game_seconds_remaining >= 300,
      season_type == "REG"
    ) %>%
    mutate(
      yards_gained = ifelse(fumble_lost == 1, -1100 - return_yards + yards_gained, yards_gained),
      yards_gained = ifelse(interception == 1, -2100 - return_yards + air_yards, yards_gained),
      yards_gained = ifelse(pass_touchdown == 1 | rush_touchdown == 1, 10 + yards_gained, yards_gained)
    ) %>%
    pull(yards_gained)

  # --- Fill final gaps with fallback vectors ---
  rush_distributions <- fill_missing_defaults(rush_distributions, all_keys, rush_pool, threshold)
  pass_distributions <- fill_missing_defaults(pass_distributions, all_keys, pass_pool, threshold)

  # --- Pad all vectors to threshold length ---
  rush_distributions <- pad_short_keys(rush_distributions, all_keys, threshold)
  pass_distributions <- pad_short_keys(pass_distributions, all_keys, threshold)

  # --- Save as JSON ---
  dir.create("distr_data", showWarnings = FALSE)
  rush_filename <- paste0("distr_data/rush_distributions_yl", yardline_range, ".json")
  pass_filename <- paste0("distr_data/pass_distributions_yl", yardline_range, ".json")

  write_json(rush_distributions, rush_filename, pretty = TRUE, auto_unbox = TRUE)
  write_json(pass_distributions, pass_filename, pretty = TRUE, auto_unbox = TRUE)
}

print("✅ All down-distance combos saved with interpolation, fallback, and padding!")
