import argparse
import cutscene.tokenizer
import cutscene.parser

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog='Material Writer',
        description='Converts a json material into a material binary'
    )

    parser.add_argument('input')
    parser.add_argument('output')

    args = parser.parse_args()
    result: cutscene.parser.Cutscene = None
    
    with open(args.input) as file:
        result = cutscene.parser.parse(file.read(), args.input)

    print(result)