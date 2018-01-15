require 'sinatra/base'
require 'webrick'
require 'webrick/https'
require 'openssl'

webrick_options = {
        :Host               => '0.0.0.0',
        :Port               => 8443,
        :Logger             => WEBrick::Log::new($stderr, WEBrick::Log::DEBUG),
        :DocumentRoot       => "/dev/null",
        :SSLEnable          => true,
        :SSLVerifyClient    => OpenSSL::SSL::VERIFY_NONE,
        :SSLCertificate     => OpenSSL::X509::Certificate.new(  File.open(File.join("server.pem")).read),
        :SSLPrivateKey      => OpenSSL::PKey::RSA.new(          File.open(File.join("server.pem")).read),
        :SSLCertName        => [ [ "CN",WEBrick::Utils::getservername ] ]
}

class MyServer  < Sinatra::Base
  i = 0

  get '/api/last-message.json' do
    i = i+1;
    case i % 3;
    when 0
      %q[{"status":"good","body":"Everything operating normally.","created_on":"2018-01-12T21:24:07Z"}]
    when 1
      %q[{"status":"minor","body":"Everything operating normally.","created_on":"2018-01-12T21:24:07Z"}]
    when 2
      %q[{"status":"major","body":"Everything operating normally.","created_on":"2018-01-12T21:24:07Z"}]
    end
    end
end

Rack::Handler::WEBrick.run MyServer, webrick_options
