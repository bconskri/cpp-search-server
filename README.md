# cpp-search-server
Проект: поисковая система

Программа для поиска по ключевым словам в добавленных ранее документах. 
Учитывает статус документа и его рейтинг, ранжирует результаты по TF-IDF с учетом стоп слов. 
main.cpp запускает тесты программы и дает представление о вариантах использования программы.


Setup for CMake

cmake_minimum_required(VERSION 3.21)
project(cpp_search_server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_VERBOSE_MAKEFILE on)
#
add_executable(cpp-search-server search-server/main.cpp search-server/tests.cpp search-server/string_processing.cpp
search-server/search_server.cpp search-server/search_server.h search-server/request_queue.cpp search-server/read_output_functions.cpp search-server/document.cpp
search-server/paginator.h search-server/test_example_functions.cpp search-server/test_example_functions.h search-server/log_duration.h
search-server/remove_duplicates.cpp search-server/remove_duplicates.h search-server/process_queries.cpp
search-server/process_queries.h Google_tests/test_par_2_3.h search-server/concurrent_map.h)