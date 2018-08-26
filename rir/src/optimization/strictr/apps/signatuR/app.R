#!../../tools/R CMD BATCH

require(shiny)
library(visNetwork)
library(shinyAce)

                                        # TODO - major refactoring needed
                                        # TODO - See if a more customizable library can be used
                                        # TODO - refactor the functions
                                        # TODO - document the functions
                                        # TODO - see if a graph analysis library can be used
                                        # TODO - preallocate arrays for speed
                                        # TODO - ensure fixed height of all tab panels

compile <- function(f) rir.compile(f)

analyse <- function(f) {
  analysis <- .Call("rir_analysis_strictness_intraprocedural", f)
  analysis$begin <- c()

  for(i in 1:length(analysis$arguments)) {
    if(all(analysis$order[,i] == "NEVER"))
      analysis$begin[i] = "ALWAYS"
    else
      analysis$begin[i] = "NEVER"
  }
  names(analysis$begin) <- analysis$arguments
  levels <- rep(-1, times = length(analysis$arguments))
  names(levels) <- analysis$arguments
  scanner <- c()
  level <- 0
  for(i in analysis$arguments) {
    if(analysis$begin[i] != "NEVER") {
      scanner <- append(scanner, i)
      levels[i] <- level
    }
  }
  index <- 1
  filler <- c()

  while(length(scanner) != 0 ) {
    level <- level + 1
    for(element in scanner) {
      for(argument in analysis$arguments) {
        if(element != argument &&
           analysis$order[element, argument] != "NEVER" &&
           levels[argument] == -1) {
          filler <- append(filler, argument)
          levels[argument] <- level
        }
      }
    }
    scanner <- filler
    filler <- c()
  }
  analysis$levels <- levels
  analysis
}

prettifyValue <- function(v) {
  switch(v,
         ALWAYS = "✓",
         NEVER = "✗",
         SOMETIMES = "?")
}

visualise <- function(analysis) {
  # TODO - track down what happens if this null check is removed
  if(is.null(analysis$order)) return(NULL)
  # color, shape, dashes
  # forced - 
  # contains - dashed vs not dashed
  # starting - star vs triangle vs circle
  # equivalence classes - colors
  from = c()
  to = c()
  dashes = c()
  title = vector(length = length(analysis$arguments))
  edgeTitle = c()
  index <- 1
  yesColor = list(background = "#C2FABC", border = "#4AD63A",
                 highlight = list(background = "#E6FFE3", border = "#4AD63A"))
  noColor = list(background = "#FB7E81", border = "#FA0A10",
                 highlight = list(background="#FFAFB1", border = "#FA0A10"))
  maybeColor = list(background = "#97C2FC", border = "#2B7CE9",
                   highlight = list(background = "#D2E5FF", border = "#2B7CE9"))
  forced = list(yes = analysis$arguments[which(analysis$forced == "ALWAYS")],
                no = analysis$arguments[which(analysis$forced == "NEVER")],
                maybe = analysis$arguments[which(analysis$forced == "SOMETIMES")])

  contains = list(yes = analysis$arguments[which(analysis$contains == "ALWAYS")],
                no = analysis$arguments[which(analysis$contains == "NEVER")],
                maybe = analysis$arguments[which(analysis$contains == "SOMETIMES")])
  last = c()
  for(i in analysis$arguments) {
    if(analysis$order[i, i] == "SOMETIMES") { last <- append(last, i); }
    title[index] <- paste0("<table>",
                           "<tr>",
                           "<th>Forced</th>",
                           "<th>&nbsp;</th>",
                           "<th>Contains</th>",
                           "</tr>",
                           "<tr>",
                           "<td style=\"text-align:center\">", prettifyValue(analysis$forced[[i]]), "</td>",
                           "<td>", "&nbsp;&nbsp;", "</td>",
                           "<td style=\"text-align:center\">", prettifyValue(analysis$contains[[i]]), "</td>",
                           "</tr>",
                           "</table>")

    for(j in analysis$arguments) {
      if(i == j) next
      direction = analysis$order[i, j]
      if(direction != "NEVER") {
        from <- append(from, i)
        to <- append(to, j)
        dashes <- append(dashes, direction == "SOMETIMES")
        edgeTitle <- append(edgeTitle, paste0("<font size=\"4\">",
                                              i,
                                              switch(direction, maybe = " ⇢ ", yes = " → "),
                                              j,
                                              "</font>"))
      }
    }
    index <- index + 1
  }
  nodes <- data.frame(id = analysis$arguments,
                      label = analysis$arguments,
                      title = title,
                      level = analysis$levels,
                      group = analysis$arguments,
                      physics = rep(FALSE, times = length(analysis$arguments)))
  edges <- data.frame(from = from, to = to, arrows = rep("middle", times = length(from)), dashes = dashes, title = edgeTitle)

  ledges <- data.frame(label = c("ALWAYS", "SOMETIMES"),
                       arrows =c("to", "to"),
                       dashes = c(FALSE, TRUE)
                       )
  lnodes <- data.frame(label = c("Never Forced", "Forced First", "Forced Last", "Forced Midway"),
                       shape = c("diamond", "triangle", "triangleDown", "square"),
                       value = rep(10, times = 4))

  graph <- visNetwork(nodes, edges, width = "100%", height = "1000px")

  for (group in contains$maybe) {
    graph <- visGroups(graph, groupname=group, borderWidth = 3,
                       borderWidthSelected = 4,
                       shapeProperties = list(borderDashes = c(5, 5)))
  }
  for (group in contains$no) {
    graph <- visGroups(graph, groupname=group, borderWidth = 0,
                       borderWidthSelected = 1)
  }
  for (group in contains$yes) {
    graph <- visGroups(graph, groupname=group, borderWidth = 3,
                       borderWidthSelected = 4)
  }
  for (group in analysis$arguments) {
    graph <- visGroups(graph, groupname=group, shape = "square")
  }
  for (group in analysis$arguments[which(analysis$begin != "NEVER")]) {
    graph <- visGroups(graph, groupname=group, shape = "triangle")
  }
  for (group in analysis$arguments[which(analysis$end != "NEVER")]) {
    graph <- visGroups(graph, groupname=group, shape = "triangleDown")
  }
  for (group in analysis$arguments[which(analysis$end != "NEVER" & analysis$begin != "NEVER")]) {
    graph <- visGroups(graph, groupname=group, shape = "diamond")
  }
  for (group in analysis$arguments[which(analysis$levels == -1)]) {
    graph <- visGroups(graph, groupname=group, shape = "star")
  }
  for (group in forced$yes) {
    graph <- visGroups(graph, groupname=group, color=yesColor)
  }
  for (group in forced$maybe) {
    graph <- visGroups(graph, groupname=group, color=maybeColor)
  }
  for (group in forced$no) {
    graph <- visGroups(graph, groupname=group, color=noColor)
  }

  #visLegend(graph, addNodes = lnodes, addEdges = ledges, position = "right", useGroups = FALSE) %>%
    visHierarchicalLayout(graph, levelSeparation=125) %>% #, treeSpacing = 100) %>%
    visOptions(highlightNearest = TRUE, nodesIdSelection = FALSE) %>%
    visInteraction(navigationButtons = TRUE)
    #visHierarchicalLayout(direction = "LR") %>%

                                        #visOptions(highlightNearest = TRUE) #list(enabled = T, degree = 2, hover = TRUE))
}

pipeline <- function(f) visualise(analyse(compile(f)))

test1 <- function() pipeline(function(x, y, z) { if(z) x + 1 else y + 1})

signatuR <- function() {

  bytecodeOutput <- function(outputId) {
    pre(id = outputId, class = "shiny-text-output", style="height: 768px;")
  }
                                        # http://stackoverflow.com/questions/1815606/rscript-determine-path-of-the-executing-script
  #print(sys.frame(1)$ofile)
  #script.dir <- dirname(sys.frame(1)$ofile)
  examplesPath <- file.path("rir/src/optimizer/strictr/apps/signatuR", "default-examples.R")
  examples <- readChar(examplesPath, file.info(examplesPath)$size)
  ui <- fluidPage(
    titlePanel("SignatuR"),
    tags$hr(),
    sidebarLayout(
      sidebarPanel(
        width = 6,
        fileInput('source',
                  'Choose an R File',

                  accept=c('text/R',
                           '.r',
                           ".R")
                  ),
        aceEditor("code", value = examples,
                  #value = "# Write some R code or choose an R File and click `Analyze`
                  #
#identity <- function(x) x
#",
                  mode="r",
                  theme = "chrome",
                  fontSize = 20,
                  showLineNumbers = TRUE,
                  highlightActiveLine = TRUE,
                  height = 700,
                  autoComplete = "live"),
        actionButton("analyze", "Analyze")
      ),
      mainPanel(
        width = 6,
        selectizeInput("functionSelector", label = NULL, choices = c("identity"), selected = NULL, width = "100%"),
         tabsetPanel(id = "tabs",
           tabPanel(title = "Bytecode", br(), bytecodeOutput("bytecode")),
           tabPanel(title = "Analysis",
                    h4("Argument Evaluation Details"), br(),
                    htmlOutput("statistics", align = "center"),
                    h4("Argument Evaluation Order (Directed Graph)"), br(),
                    htmlOutput("directions", align = "center")),
           #tabPanel(title = "Directions", br(), br(), br(), htmlOutput("directions", align = "center")),
           tabPanel(title = "Visualization", visNetworkOutput("network", height = 790)))
        # dataTableOutput("data")
      )
    )
  )

  prettify <- function(v) {
    for(name in names(v)) {
      v[[name]] <- switch(v[[name]],
                          ALWAYS = "✓",
                          NEVER = "✗",
                          SOMETIMES = "?")
    }
    v
  }

  prettifyDirections <- function(matrix) {
    if(is.null(matrix) || nrow(matrix) == 0 || ncol(matrix) == 0) return(matrix)
    for (r in 1:nrow(matrix)) {
      for (c in 1:ncol(matrix)) {
        matrix[r, c] <- switch(matrix[r, c],
                               ALWAYS = "✓",
                               NEVER = "✗",
                               SOMETIMES = "?")
      }
    }
    matrix
  }
  server <- function(input, output, session) {

    observe({
      infile <- input$source
      if(is.null(infile)) {
        return(NULL)
      }
      content <- readChar(infile$datapath, file.info(infile$datapath)$size)
      updateAceEditor(session,
                      "code",
                      value = content)
    })

    cache <- reactive({
      input$analyze
      environment <- new.env(hash = TRUE)
      result <- new.env(hash = TRUE)
      tryCatch({
        isolate(eval(parse(text=input$code), envir = environment))
        removeNotification("error")
        removeNotification("warning")
      },
      error = function(e) {
        showNotification(paste(e), id="error", type = "error", duration = NULL)
      },
      warning = function(w) {
        showNotification(paste(e), id="warning", type = "warning", duration = NULL)
      })

      functionNames = c()
      for(name in ls(environment)) {
        obj = environment[[name]]
        if(typeof(obj)== "closure") {
          functionNames = append(functionNames, name)
          binary = compile(obj)
          result[[name]] = list(binary = binary, analysis = analyse(binary))
        }
      }
      if(input$functionSelector %in% functionNames) selected = input$functionSelector
      else selected = functionNames[1]
      if(length(functionNames) == 0) functionNames = c("")
      updateSelectInput(session, "functionSelector", choices = functionNames, selected = selected)
      #validate(need(length(functionNames) != 0, "No accessible function definition"))
      result
    })

    functionObject <- reactive({
      name = input$functionSelector
      if(name == "") name = "a"
      cache()[[name]]
    })

    output$bytecode <- renderText({
      if(is.null(functionObject())) return("No accessible function definition")
      paste(capture.output(rir.disassemble(functionObject()$binary), file = NULL), collapse = "\n")
    })

    # DataTable
    output$statistics <- renderTable({
      analysis <- functionObject()$analysis
      print(analysis)
      data.frame(Arguments = analysis[["arguments"]],
                 Begin = prettify(analysis[["begin"]]),
                 #End = prettify(analysis[["end"]]),
                 Contains = prettify(analysis[["contains"]]),
                 Forced = prettify(analysis[["forced"]]),
                 Level = as.integer(analysis[["levels"]]))
    })
    output$directions <- renderTable({
      prettifyDirections(functionObject()$analysis$order)
    },
    rownames = TRUE,
    colnames = TRUE,
    align = 'c',
    striped = TRUE,
    hover = TRUE,
    bordered = TRUE
    )

    output$network <- renderVisNetwork({
      visualise(functionObject()$analysis)
    })
  }
  shinyApp(ui = ui, server = server)
}
