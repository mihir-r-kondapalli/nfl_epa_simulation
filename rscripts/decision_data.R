#!/usr/bin/env Rscript

options(repos = c(CRAN = "https://cran.rstudio.com/"))
library(nflfastR)
library(tidyverse)
library(jsonlite)

# ========== 1) LOAD PLAY-BY-PLAY & PREP ==========

args <- commandArgs(trailingOnly = TRUE)
if (length(args) < 1) {
  stop("Usage: Rscript script.R <output directory>")
}
output_dir <- args[1]

# NFL fallback cache file
nfl_cache_file <- paste0(output_dir, "/nfl_decision_counts.csv")

pbp_data_nfl <- load_pbp(2019:2024) %>%
    filter(game_seconds_remaining >= 300, play_type %in% c("run", "pass", "field_goal", "punt")) %>%
    mutate(play_category = case_when(
      play_type == "run" & qb_scramble == 0 ~ 0L,
      play_type == "pass" | qb_scramble == 1 ~ 1L,
      play_type == "field_goal"             ~ 2L,
      play_type == "punt"                   ~ 3L,
      TRUE                                  ~ NA_integer_
    )) %>%
    filter(!is.na(play_category)) %>%
    count(down, ydstogo, yardline_100, play_category, name = "n") %>%
    pivot_wider(names_from = play_category, values_from = n, values_fill = 0) %>%
    rename(run = `0`, pass = `1`, kick = `2`, punt = `3`) %>%
    mutate(key = paste(down, ydstogo, yardline_100, sep = "-"))
  
  write_csv(pbp_data_nfl, nfl_cache_file)
  nfl_counts_all <- pbp_data_nfl

print(paste("Decision Data Generated: ", nfl_cache_file))