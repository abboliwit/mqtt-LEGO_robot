var mqtt = require('mqtt');// förklarar variandeln som ett bibliotek
var client = mqtt.connect("mqtt:/192.168.0.140",{clientId: "lyssnarlasse",clean:false});// förklarar vart den ska Subscriba för att ta in medelande

client.on("connect",function(){
    client.subscribe({"mess":0,"json":0,"offline":2});

});

client.on("message", function(topic,message){//när vi får ettt medelande så kollar vi om det är något som vi vill ha
    var context = message.toString();//förklarar context som medelandet
    if (topic == "json"){// om det är ett topic som vi vill ha så loggar vi det för att enkelt kunna felsöka 
        var json = JSON.parse(context);
        console.log(json.dis+" på "+json.pos)
    }
    if (topic == "offline"){// om topicet är offline så säger vi att denförsvann
        console.log(context+" har gått vidare")
    }

    else{
        
        console.log(context);// om det inte är nåtutav det så kommer den skriva ut medelandet
    }

    
})// sub behövs igentligen inte den gör det endast enklare att felsöka 