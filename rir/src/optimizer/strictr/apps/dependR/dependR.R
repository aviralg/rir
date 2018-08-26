require(shiny)
require(visNetwork)

options(repos = c(CRAN = "http://cran.us.r-project.org"))

#print(jsonlite::toJSON(available))

populate_environment <- function(available, packages) {

  vectorize <- function(value) {
    if(is.na(value)) return(NULL)
    result <- unlist(strsplit(value, split="\\s*,\\s*"))
    for(i in 1:length(result)) {
      result[i] = unlist(strsplit(result[i], split = "\\s*\\("))[1]
    }
    result
  }
  package_to_list <- function(row) {
    package <- list()
    for(name in names(row)) {
      package[[name]] <- unname(row[name])
    }
    package[["Suggests"]] <- vectorize(package[["Suggests"]])
    package[["Enhances"]] <- vectorize(package[["Enhances"]])
    package[["Imports"]]  <- vectorize(package[["Imports"]])
    package[["Depends"]]  <- vectorize(package[["Depends"]])
    package[["level"]] <- -1
    package
  }
  for(row in 1:nrow(available)) {
    assign(available[row, "Package"], package_to_list(available[row,]), packages)
  }
  packages
}

analyze_dependencies <- function(available, packages) {
}

print_environment <- function(packages) {
  for (name in ls(packages)) {
    print(packages[[name]])
  }
}


assign_levels <- function(packages) {
  print(packages[["tools"]])
  compute_level <- function(package_name) {
    package_details <- packages[[package_name]]
    if (is.null(package_details)) return (1)
    l = package_details[["level"]]
    if( l != -1) return(l)
    l = 0
    for(p in package_details[["Imports"]]) {
      l <- max(l, compute_level(p))
    }
    package_details[["level"]] <- (l + 1)
    packages[[package_name]] <- package_details
    return (l + 1)
  }
  for(package_name in ls(packages)) {
    compute_level(package_name)
  }
  packages
}

increment_count <- function(packages) {
  for(package_name in ls(packages)) {
    package_details <- packages[[package_name]]
    for(p in package_details[["Imports"]]) {
      packages[[p]][[count]] <- packages[[p]][["Dependents"]] + 1
    }
  }
}


generate_dependency_graph <- function() {
  assign_levels(populate_environment(available.packages(),
                                     new.env(hash = TRUE)))
}




dependency_visualizer <- function() {
  packages <- generate_dependency_graph()
  server <- function(input, output) {
    r <- print(unlist(ls(packages)))
    r <- r[1:100]

    output$network <- renderVisNetwork({
                                        # minimal example
      nodes <- data.frame(id=1:length(r),
                          label=r)

      edges <- data.frame(from = c(), to = c())
      visNetwork(nodes, edges, width="100%", height="inherit")
    })
  }

  ui <- fluidPage(
    visNetworkOutput("network")
  )

  shinyApp(ui = ui, server = server)
}

dependency_visualizer()
