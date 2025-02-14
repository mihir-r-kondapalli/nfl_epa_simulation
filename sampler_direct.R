# Load necessary libraries
options(repos = c(CRAN = "https://cran.rstudio.com/"))
library(jsonlite)
library(tidyverse)

# Generate individual bins for yardlines 1-20
individual_bins <- setNames(as.list(1:20), as.character(1:20))

# Define grouped bins for higher yardlines
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

# Combine both individual and grouped bins
yardline_bins <- c(individual_bins, grouped_bins)

# Define play types (Passing and Rushing)
play_types <- c("pass", "rush")

# Function to sample a value using inverse transform sampling
sample_from_cdf <- function(cdf_table, n = 1) {
  random_values <- runif(n)  # Generate 'n' random numbers between 0 and 1

  # Find the smallest value where CDF is greater than the random number
  sampled_values <- sapply(random_values, function(u) {
    cdf_table$value[which.min(cdf_table$cdf < u)]  # Find corresponding value
  })
  
  return(sampled_values)
}

# Function to sample values directly from the dataset
sample_from_existing_data <- function(data, n = 10000) {
  sampled_results <- list()
  
  for (down_distance in names(data)) {
    values <- data[[down_distance]]  # Extract values from JSON
    values <- discard(values, is.null)
    values <- values[!is.na(values)]    # Remove NA values

    
    if (length(values) > 20) {
      sampled_results[[down_distance]] <- sample(values, n, replace = TRUE)  # Sample with replacement
    } else {
      sampled_results[[down_distance]] <- numeric(0)  # Return empty array if no values exist
    }
  }
  
  return(sampled_results)
}

# Process both passing and rushing data
for (play_type in play_types) {
  for (group in names(yardline_bins)) {  # Yardline bins are only used for file naming
    # Construct file path based on play type & yardline group
    cdf_file <- paste0("distr_data/", play_type, "_distributions_yl", group, ".json")
    
    # Check if file exists
    if (!file.exists(cdf_file)) {
      print(paste("Skipping missing file:", cdf_file))
      next
    }
    
    # Load JSON CDF data
    cdf_data <- fromJSON(cdf_file)
    
    # Sample from all down & distance keys
    set.seed(31)  # Ensure reproducibility
    all_samples <- sample_from_existing_data(cdf_data, 10000)
    
    # Save sampled data to JSON
    output_file <- paste0("sample_data/", play_type, "_sample_yl", group, ".json")
    write_json(all_samples, output_file, pretty = TRUE)
    
    print(paste("Finished sampling for", play_type, "yardline group:", group))
  }
}
