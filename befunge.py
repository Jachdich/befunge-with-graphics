import sys, random, pygame, time

dbg = "-d" in sys.argv

def makeGrid(code):
    out = []
    for line in code.split("\n"):
        out.append([char for char in line])

    return out

def pad(grid):
    out = []
    m = max([len(i) for i in grid])
    for row in grid:
        while len(row) < m:
            row.append(" ")
        out.append(row)
    return out

class Interpreter:
    def __init__(self, code):
        self.code = code
        self.grid = pad(makeGrid(self.code))
        self.ip = [0, 0]
        self.direction = "right"
        self.stringMode = False
        self.running = True
        self.screen = None

        self.stack = []
        self.a = time.time()
        self.ret_stack = []

    def step(self):
        print(len(self.stack))
        if dbg and False:
            input()
        curr_char = self.grid[self.ip[1]][self.ip[0]]
        if dbg:
            print(self.stack, self.ip)

        if curr_char == "\"":
            self.stringMode = not self.stringMode

        if not self.stringMode:

            if curr_char.isdigit():
                self.push(int(curr_char))

            elif curr_char == "j":
                y = self.pop()
                x = self.pop()
                d = self.direction
                x2 = self.ip[0]
                y2 = self.ip[1]
                if d == "right":
                    x2 += 1
                elif d == "left":
                    x2 -= 1
                elif d == "up":
                    y2 -= 1
                elif d == "down":
                    y2 += 1
                    
                
                self.ret_stack = [{"right": 0, "left": 1, "up": 2, "down": 3}[self.direction], x2, y2]
                self.direction = "right"
                self.ip = [x - 1, y]

            elif curr_char == "r":
                y = self.ret_stack.pop()
                x = self.ret_stack.pop()
                d = {0: "right", 1: "left", 2: "up", 3: "down"}[self.ret_stack.pop()]
                if d == "right":
                    x -= 1
                elif d == "left":
                    x += 1
                elif d == "up":
                    y += 1
                elif d == "down":
                    y -= 1

                self.direction = d
                self.ip = [x, y]

            elif curr_char == "s":
                y = self.pop()
                x = self.pop()
                self.screen = pygame.display.set_mode((x, y))
                
            elif curr_char == "x":
                y = self.pop()
                x = self.pop()
                pygame.draw.line(self.screen, (0, 0, 0), (x, y), (x, y))

            elif curr_char == "c":
                self.screen.fill((255, 255, 255))

            elif curr_char == "u":
                pygame.display.update()

            elif curr_char == "z":
                event = pygame.event.poll()
                if event.type == pygame.KEYDOWN:
                    self.push(event.key)

                self.push(event.type)
                if False:
                    print(1.0 / (time.time() - self.a))
                    self.a = time.time()

            elif curr_char == "+":
                a = self.pop()
                b = self.pop()
                self.push(a + b)

            elif curr_char == "-":
                a = self.pop()
                b = self.pop()
                self.push(b - a)

            elif curr_char == "*":
                a = self.pop()
                b = self.pop()
                self.push(a * b)

            elif curr_char == "/":
                a = self.pop()
                b = self.pop()
                self.push(b // a)

            elif curr_char == "%":
                a = self.pop()
                b = self.pop()
                self.push(b % a)

            elif curr_char == "!":
                a = self.pop()
                if a == 0:
                    self.push(1)
                else:
                    self.push(0)

            elif curr_char == "`":
                a = self.pop()
                b = self.pop()
                if b > a:
                    self.push(1)
                else:
                    self.push(0)
                
            elif curr_char == ".":
                a = self.pop()
                sys.stdout.write(str(a) + " ")

            elif curr_char == ",":
                a = self.pop()
                sys.stdout.write(chr(a))
                sys.stdout.flush()

            elif curr_char in "<>v^":
                self.direction = {"<": "left", ">": "right", "v": "down", "^": "up"}[curr_char]

            elif curr_char == "?":
                self.direction = ["left", "right", "up", "down"][random.randint(0, 3)]

            elif curr_char == "_":
                a = self.pop()
                if a == 0:
                    self.direction = "right"
                else:
                    self.direction = "left"

            elif curr_char == "|":
                a = self.pop()
                if a == 0:
                    self.direction = "down"
                else:
                    self.direction = "up"

            elif curr_char == ":":
                a = self.pop()
                self.push(a)
                self.push(a)

            elif curr_char == "\\":
                a = self.pop()
                b = self.pop()
                self.push(a)
                self.push(b)

            elif curr_char == "&":
                num = 0
                inp = sys.stdin.readline()
                try:
                    num = int(inp)
                except ValueError:
                    pass

                self.stack.append(num)

            elif curr_char == "&":
                inp = sys.stdin.readline()

                if len(inp) != 1:
                    inp = chr(0)

                self.stack.append(ord(inp))

            elif curr_char == "#":
                self.advance()

            elif curr_char == "g":
                try:
                    y = self.pop()
                    x = self.pop()
                    self.push(ord(self.grid[y][x]))
                except IndexError:
                    self.push(0)

            elif curr_char == "p":
                y = self.pop()
                x = self.pop()
                v = chr(self.pop())

                try:
                    self.grid[y][x] = v
                except IndexError:
                    if y >= len(self.grid):
                        while y >= len(self.grid):
                            self.grid.append([])
                    else:
                        if x >= self.grid[y]:
                            while x >= self.grid[y]:
                                self.grid[y].append(" ")

                    self.grid = pad(self.grid)

                    self.grid[y][x] = v

            elif curr_char == "$":
                self.pop()
        
            elif curr_char == "@":
                self.running = False
                return

        elif curr_char != "\"":
            self.push(ord(curr_char))
        
        self.advance()

    def advance(self):
        if self.direction == "right":
            self.ip[0] += 1
        elif self.direction == "left":
            self.ip[0] -= 1
        elif self.direction == "up":
            self.ip[1] -= 1
        elif self.direction == "down":
            self.ip[1] += 1

    def run(self):
        while self.running:
            self.step()

    def push(self, item):
        self.stack.append(item)

    def pop(self):
        try:
            return self.stack.pop()
        except:
            return 0

if len(sys.argv) < 2:
    print("Error: expected filename")
    sys.exit(1)

with open(sys.argv[1], "r") as f:
    s = f.read()

i = Interpreter(s)

i.run()
