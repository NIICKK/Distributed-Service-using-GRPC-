#include "threadpool.h"

#include <memory>
#include <iostream>
#include <string>
#include <fstream>

#include <grpc++/grpc++.h>
#include <grpc/support/log.h>
#include "store.grpc.pb.h"
#include "vendor.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;

using store::Store;
using store::ProductQuery;
using store::ProductReply;
using store::ProductInfo;

using vendor::BidQuery;
using vendor::BidReply;
using vendor::Vendor;

//updated

//Store class
class MyStore final { 
  //define variables
  private:
  	Store::AsyncService service_;
        std::string server_address_;
  	std::unique_ptr<Server> server_;
  	std::unique_ptr<ServerCompletionQueue> cq_;
	std::vector<std::string> vendors_;
	ThreadPool pool_;
  public:
  	//constructor
    	MyStore(const std::string& server_address, int MAX_THREAD_NUMBER) : pool_(MAX_THREAD_NUMBER){
		server_address_ = server_address;
    	}
    
	//destructor
	~MyStore(){
		server_->Shutdown();
		cq_->Shutdown();		
	}
   
   	//run
	void Run(){
		ServerBuilder builder;
		//Listen on server address
		builder.AddListeningPort(server_address_, grpc::InsecureServerCredentials());
		//Register the service
		builder.RegisterService(&service_);
		//Get hold of completion Queue
		cq_ = builder.AddCompletionQueue();
		//Assemble the server
		server_ = builder.BuildAndStart();
		HandleRpcs();	
	}
  
	//add a vendor into vendor list
	void addVendor(const std::string& vendor){
		vendors_.push_back(vendor);	
	}
  private:
    //RPC handler 
 	void HandleRpcs(){
		//TODO [done]
		new CallData(&service_, cq_.get(), vendors_);
		void* tag;
            	bool ok;
		while (true){
                	GPR_ASSERT(cq_->Next(&tag, &ok));
                	GPR_ASSERT(ok);
                	//assign a new thread into threadpool
			pool_.addTask([tag](){static_cast<CallData*>(tag)->Proceed();});
            	}
	}
    //call data class
    class CallData{
      //define variables
      private:
	Store::AsyncService* service_;
	ServerCompletionQueue* cq_;
	ServerContext ctx_;
	ProductQuery request_;
        ProductReply reply_;
	ServerAsyncResponseWriter<ProductReply> responder_;
	enum CallStatus { CREATE, PROCESS, FINISH };
    	CallStatus status_; 
	std::vector<std::string> vendors_;	//each call data should have a copy of vendor list rather than shared
      public:
	//constructor
	CallData(Store::AsyncService* service, ServerCompletionQueue* cq, std::vector<std::string>& vendors) : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE) {
		vendors_.assign(vendors.begin(), vendors.end());	//copy vendors
      		Proceed();
    	}

	//proceed
	void Proceed(){
		//CREATE state
		if (status_ == CREATE) {
        		//make this instance progress to the PROCESS state.
        		status_ = PROCESS;
			//requests list of prices from store, for different vendors registered at the store
			service_->RequestgetProducts(&ctx_, &request_, &responder_, cq_, cq_, this);
		//PROCESS state
      		} else if (status_ == PROCESS) {
			//TODO
			new CallData(service_, cq_, vendors_);
			//the actual processing
			//for each vendor
			for (std::vector<std::string>::iterator it = vendors_.begin(); it != vendors_.end(); ++it){
			    //get the stub
			    std::unique_ptr<Vendor::Stub> stub(Vendor::NewStub(grpc::CreateChannel(*it, grpc::InsecureChannelCredentials())));
			    //bid variables
   			    BidQuery bidQuery;
			    BidReply bidReply;
			    ClientContext clientContext;
			    //send request
    			    bidQuery.set_product_name(request_.product_name());
			    //RPC call HERE
			    Status bidStatus = stub->getProductBid(&clientContext, bidQuery, &bidReply);
			    if (bidStatus.ok()){
                            	if (!bidReply.vendor_id().empty()){
                            		ProductInfo* info = reply_.add_products();
                            		info->set_price(bidReply.price());
                            		info->set_vendor_id(bidReply.vendor_id());
			    	}
			    }
			    else{
			    	std::cout << bidStatus.error_code() << ": " << bidStatus.error_message() << std::endl;
			    }
                        }
                        status_ = FINISH;
                        responder_.Finish(reply_, Status::OK, this);
		//FINISH state
      		} else {	
                        GPR_ASSERT(status_ == FINISH);
                        delete this;
      		}
	}
    };
};

int main(int argc, char** argv) {
	std::string server_address = "0.0.0.0:50056";
	int MAX_THREAD_NUMBER = 8;
	if(argc != 3){
		printf("Usage: ./store [server_address:port] [max_thread_number]\n");	
	}
	else{
		server_address = std::string(argv[1]);
		MAX_THREAD_NUMBER = atoi(argv[2]);
	}
	MyStore myStore(server_address, MAX_THREAD_NUMBER);
	//TODO: read vendor addresses [done]
	std::ifstream file;
	file.open("vendor_addresses.txt");
    	if (!file){
		std::cout << "Failed to open vendor_addresses.txt" << std::endl;
		exit(1);	
	}
    	std::string vendor;
	while (getline(file,vendor)){
            	myStore.addVendor(vendor);
        }
	file.close();
	//RUN
	myStore.Run();
	return EXIT_SUCCESS;
}
