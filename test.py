import random
import string

TESTS_DIR           = "./tests/"
TEST_FILE_PREFIX    = "t_"
TEST_TYPE           = ".txt"

STR_LEN                 = int(input("String len: "))
INITAL_VOCABULATY_LEN   = int(input("Vocabulary len: "))
PLAY_LEN                = int(input("Play len: "))
NEW_ADDITIONS           = int(input("New additions: "))
ALLOWED_CHARS           = string.ascii_lowercase + string.ascii_uppercase + string.digits + "_" + "-"

def random_string_generator():
    return ''.join(random.choice(ALLOWED_CHARS) for x in range(STR_LEN))

def generare_random_word_list(size):
    ret = [random_string_generator() for x in range(size)]
    return ret

with open (f'{TESTS_DIR}/{TEST_FILE_PREFIX}1{TEST_TYPE}', "w") as test:
    test.write(str(INITAL_VOCABULATY_LEN) + "\n")
    vocabulary = generare_random_word_list(INITAL_VOCABULATY_LEN)
    for word in vocabulary:
        test.write(word + "\n")
    test.write("+nuova_partita\n")
    # target
    test.write(random_string_generator() + "\n")
    test.write(str(PLAY_LEN) + "\n")
    word_list = generare_random_word_list(PLAY_LEN)
    for word in word_list:
        test.write(word + "\n")

    # add new matches
    for i in range(NEW_ADDITIONS):
        test.write("+inserisci_inizio\n")
        vocabulary = generare_random_word_list(INITAL_VOCABULATY_LEN)
        for word in vocabulary:
            test.write(word + "\n")
        test.write("+inserisci_fine\n")
        test.write("+nuova_partita\n")
        # target
        test.write(random_string_generator() + "\n")
        test.write(str(PLAY_LEN) + "\n")
        word_list = generare_random_word_list(PLAY_LEN)
        for word in word_list:
            test.write(word + "\n")

