print("Hello World<hr>")
print("This is a random number, i. e. dynamic content: ", math.random(10), "<hr>")


for i,v in pairs(_GET) do
	print(i, ": ", v);
end