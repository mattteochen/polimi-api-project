import os
import random
import string

TESTS_DIR           = "./tests/"
TEST_FILE_PREFIX    = "t_"
TEST_TYPE           = ".txt"

STR_LEN = input("String len: ")
INITAL_VOCABULATY_LEN = input("Vocabulary len: ")
PLAY_LEN = input("Play len: ")
ALLOWED_CHARS = string.ascii_lowercase + string.ascii_uppercase + string.digits + "_" + "-"

def random_string_generator():
    return ''.join(random.choice(ALLOWED_CHARS) for x in range(STR_LEN))

def generare_random_word_list(size):
    ret = [random_string_generator() for x in range(size)]
    return ret

with open (f'{TESTS_DIR}/{TEST_FILE_PREFIX}1{TEST_TYPE}', "w") as test:
    test.write(INITAL_VOCABULATY_LEN)
    vocabulary = generare_random_word_list(INITAL_VOCABULATY_LEN)
    for word in vocabulary:
        test.write(word)
    test.write("+nuova_partita")
    # target
    test.write(random_string_generator())
    test.write(PLAY_LEN)
    word_list = generare_random_word_list(PLAY_LEN)
    for word in word_list:
        test.write(word)

