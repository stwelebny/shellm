# shellm
<H2> shellm - Linux Terminal Client for ChatGPT.</H2> 

<DIV>Features a universal Linux commandline tool for ChatGPT. It can be considered for DevOps or AI-backed automation tasks. You can ask ChatGPT to configure your software or search something or even access the internet to assist you.</DIV>
</BR>
<B> ATTENTION:The AI gets access to your operating system with all rights of the user that started the client. shellm asks before it executes a command, but it is still recommendable to run it with limited user rights or in safe environments, e.g. sandboxed.</B>
</BR>
</BR>
<DIV>Build it with make. (I have built it successfully on Mac and on Raspberry Pi.)</DIV>
<DIV>copy chat.~conf to .chat.conf and edit your API key. You can also change the model name there.</DIV>
<DIV>Usage: ./shellm filename.chat where filename.chat is the place the chat history will be saved.</DIV>

## Support

For development support you may contact us at 

https://www.welebny.com

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Third-party Libraries

This software links the following third-party libraries:

- `jsoncpp` 
- `libcurl`

