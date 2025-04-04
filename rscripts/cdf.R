# Load necessary libraries
options(repos = c(CRAN = "https://cran.rstudio.com/"))
library(jsonlite)
library(tidyverse)

# Read threshold argument from command line
args <- commandArgs(trailingOnly = TRUE)
threshold <- if (length(args) == 0) 20 else as.numeric(args[1])

# Define yardline bins
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
play_types <- c("pass", "rush")

# Helper: convert a numeric vector into CDF format
convert_to_cdf <- function(vec) {
  tab <- sort(table(vec))
  values <- as.integer(names(tab))
  probs <- as.numeric(tab) / sum(tab)
  cdf <- cumsum(probs)
  list(values = values, cdf = cdf)
}

# Loop through each play type and yardline group
for (play_type in play_types) {
  for (group in names(yardline_bins)) {
    input_file <- paste0("distr_data/", play_type, "_distributions_yl", group, ".json")
    
    if (!file.exists(input_file)) {
      message("Skipping missing file: ", input_file)
      next
    }

    # Load raw distribution data (sample vectors)
    distr_data <- fromJSON(input_file)
    
    # Convert each key's vector to a CDF object
    cdf_data <- list()
    for (key in names(distr_data)) {
      vec <- distr_data[[key]]
      vec <- vec[!is.na(vec)]
      if (length(vec) >= threshold) {
        cdf_data[[key]] <- convert_to_cdf(vec)
      } else {
        cdf_data[[key]] <- list(values = as.integer(0), cdf = as.numeric(1))
      }
    }

    # Save output CDF JSON
    dir.create("cdf_data", showWarnings = FALSE)
    output_file <- paste0("cdf_data/", play_type, "_cdf_yl", group, ".json")
    write_json(cdf_data, output_file, pretty = TRUE, auto_unbox = TRUE)

    message("✅ Finished generating CDF for ", play_type, " yardline group: ", group)
  }
}

message("✅ All CDF generation complete.")
